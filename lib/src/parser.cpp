module;

#include <cassert>

module clap.parser;

#pragma region OptionParser

namespace clap {
namespace {
struct ArgSplit {
	[[nodiscard]] static constexpr auto create(std::string_view const word) {
		auto ret = ArgSplit{.left = strip_quotes(word)};
		if (auto const i = ret.left.find('='); i != std::string_view::npos) {
			ret.right = ret.left.substr(i + 1);
			ret.left = ret.left.substr(0, i);
		}
		ret.right = strip_quotes(ret.right);
		return ret;
	}

	std::string_view left{};
	std::string_view right{};
};

[[nodiscard]] auto or_iterate(Result const result, OptionParser::Iterate const it) -> OptionParser::Action {
	if (result.should_early_exit()) {
		return result;
	}
	return it;
}
} // namespace

OptionParser::OptionParser(IContext& context, std::span<parameter::Named const> options)
	: m_context(context),
	  m_options(options) {}

auto OptionParser::parse_next(Token const& token) -> Action {
	if (m_force_ended) {
		return Iterate::Pass;
	}

	if (m_expects_value) {
		if (token.type == Token::Type::String) {
			auto const ret = assign_value_to(*m_expects_value, token.lexeme, m_option_lexeme);
			m_expects_value = {};
			m_option_lexeme = {};
			return or_iterate(ret, Iterate::Consume);
		}

		m_context.print_error(std::format("option '{}' requires an argument", m_option_lexeme));
		return Result::create_early_exit(ExitCode::OptionRequiresArgument);
	}

	switch (token.type) {
	case Token::Type::String: return Iterate::Pass;
	case Token::Type::MinusMinus: m_force_ended = true; return Iterate::Consume;
	case Token::Type::MinusMinusString: return parse_long(token);
	case Token::Type::MinusString: return parse_shorts(token);
	default: break;
	}

	m_context.print_error(std::format("InternalError: unexpected Token: '{}'", token.lexeme));
	return Result::create_early_exit(ExitCode::InternalError);
}

auto OptionParser::finish() -> Result {
	if (!m_expects_value) {
		return Result::create_continue();
	}
	m_context.print_error(std::format("option '{}' requires an argument", m_option_lexeme));
	return Result::create_early_exit(ExitCode::OptionRequiresArgument);
}

auto OptionParser::parse_long(Token const& token) -> Action {
	auto const word = token.lexeme.substr(2);

	if (word == "version" && m_context.has_version()) {
		m_context.print_version();
		return Result::create_early_exit(ExitCode::Success);
	}

	if (word == "help") {
		m_context.print_help_text();
		return Result::create_early_exit(ExitCode::Success);
	}

	auto const split = ArgSplit::create(word);
	auto const* named = find_named(split.left);

	if (!named) {
		m_context.print_error(std::format("unrecognized option: '{}'", token.lexeme));
		return Result::create_early_exit(ExitCode::UnrecognizedOption);
	}

	auto const ret = parse_option_value(*named, split.right, token.lexeme);
	return or_iterate(ret, Iterate::Consume);
}

auto OptionParser::parse_shorts(Token const& token) -> Action {
	auto const word = token.lexeme.substr(1);
	auto const split = ArgSplit::create(word);
	auto letters = split.left;
	for (; letters.size() > 1; letters.remove_prefix(1)) {
		auto const* named = find_named(letters.front());
		if (!named) {
			return Result::create_early_exit(ExitCode::UnrecognizedOption);
		}

		if (!named->is_flag) {
			m_context.print_error(std::format("option requires an argument -- '{}'", named->letter));
			return Result::create_early_exit(ExitCode::OptionRequiresArgument);
		}

		named->parse("true");
	}

	auto const* named = find_named(letters.front());
	if (!named) {
		return Result::create_early_exit(ExitCode::UnrecognizedOption);
	}

	auto const ret = parse_option_value(*named, split.right, token.lexeme);
	return or_iterate(ret, Iterate::Consume);
}

auto OptionParser::parse_option_value(parameter::Named const& named,
									  std::string_view const value,
									  std::string_view const option_lexeme) -> Result {
	if (!named.is_flag && value.empty()) {
		m_expects_value = &named;
		m_option_lexeme = option_lexeme;
		return Result::create_continue();
	}

	if (!value.empty()) {
		return assign_value_to(named, value, option_lexeme);
	}

	if (!named.is_flag) {
		m_context.print_error(std::format("option '{}' requires an argument", option_lexeme));
		return Result::create_early_exit(ExitCode::OptionRequiresArgument);
	}

	named.parse("true");
	return Result::create_continue();
}

auto OptionParser::assign_value_to(parameter::Named const& named,
								   std::string_view const value,
								   std::string_view const option_lexeme) const -> Result {
	if (!named.parse(value)) {
		m_context.print_error(std::format("invalid '{}': {}", option_lexeme, value));
		return Result::create_early_exit(ExitCode::InvalidArgument);
	}

	return Result::create_continue();
}

template <typename T>
auto OptionParser::find_named(T const t) const -> parameter::Named const* {
	auto const pred = [t](parameter::Named const& param) {
		if constexpr (std::same_as<T, char>) {
			return param.letter == t;
		} else {
			return param.word == t;
		}
	};
	auto const it = std::ranges::find_if(m_options, pred);
	if (it != m_options.end()) {
		return &*it;
	}

	m_context.print_error(std::format("unrecognized option: '{}'", t));

	return nullptr;
}
} // namespace clap

#pragma endregion

#pragma region ParameterParser

namespace clap {
auto ParameterParser::set_parameters(ParameterList parameters) -> Result {
	clear_parameters();

	auto const visitor = Visitor{
		[this](parameter::Named named) {
			add_option(std::move(named));
			return Result::create_continue();
		},
		[this](parameter::Positional positional) {
			return add_positional(std::move(positional));
		},
		[this](parameter::List list) {
			return add_list(std::move(list));
		},
	};
	for (auto& parameter : parameters) {
		auto const result = std::visit(visitor, std::move(parameter));
		if (result.should_early_exit()) {
			clear_parameters();
			return result;
		}
	}

	return Result::create_continue();
}

auto ParameterParser::parse(std::span<std::string_view const> args) -> Result {
	m_argument_tokens.clear();
	m_arguments = {};

	auto option_parser = OptionParser{m_context, m_options};
	auto scanner = Scanner{args};
	auto token = ScannerToken{};

	auto result = Result::create_continue();
	while (scanner.scan_next(token)) {
		auto action = option_parser.parse_next(token);

		auto const visitor = Visitor{
			[&](OptionParser::Iterate const it) {
				if (it == OptionParser::Iterate::Pass) {
					m_argument_tokens.push_back(token);
				}
			},
			[&](Result const in) {
				result = in;
			},
		};
		std::visit(visitor, action);

		if (result.should_early_exit()) {
			return result;
		}
	}

	result = option_parser.finish();
	if (result.should_early_exit()) {
		return result;
	}

	return parse_arguments();
}

void ParameterParser::clear_parameters() {
	m_options.clear();
	m_list.reset();
	m_pre_list.clear();
	m_post_list.clear();
	m_optional_positionals = false;
}

void ParameterParser::add_option(parameter::Named option) { m_options.push_back(std::move(option)); }

auto ParameterParser::add_positional(parameter::Positional positional) -> Result {
	if (m_optional_positionals && positional.type == parameter::Type::Required) {
		m_context.print_error(std::format("required positional after optional(s): '{}'", positional.name));
		return Result::create_early_exit(ExitCode::InvalidParameter);
	}

	auto& target = m_list ? m_post_list : m_pre_list;
	target.push_back(std::move(positional));
	if (positional.type == parameter::Type::Optional) {
		m_optional_positionals = true;
	}

	return Result::create_continue();
}

auto ParameterParser::add_list(parameter::List list) -> Result {
	if (m_list) {
		m_context.print_error(std::format("only one list parameter is permitted: '{}'", list.name));
		return Result::create_early_exit(ExitCode::InvalidParameter);
	}

	m_list.emplace(std::move(list));
	return Result::create_continue();
}

auto ParameterParser::parse_arguments() -> Result {
	m_arguments = m_argument_tokens;
	auto result = parse_front_to_list();
	if (result.should_early_exit()) {
		return result;
	}
	result = parse_back_to_list();
	if (result.should_early_exit()) {
		return result;
	}
	result = parse_list();
	if (result.should_early_exit()) {
		return result;
	}

	if (m_arguments.empty()) {
		return Result::create_continue();
	}

	m_context.print_error(std::format("unknown argument: '{}'", m_arguments.front().lexeme));
	return Result::create_early_exit(ExitCode::UnknownArgument);
}

auto ParameterParser::parse_front_to_list() -> Result {
	for (auto const& positional : m_pre_list) {
		if (m_arguments.empty()) {
			return check_required(positional);
		}

		auto const argument = m_arguments.front();
		m_arguments = m_arguments.subspan(1);
		if (!parse_positional(positional, argument.lexeme)) {
			return Result::create_early_exit(ExitCode::InvalidArgument);
		}
	}

	return Result::create_continue();
}

auto ParameterParser::parse_back_to_list() -> Result {
	for (auto const& positional : std::views::reverse(m_post_list)) {
		if (m_arguments.empty()) {
			return check_required(positional);
		}

		auto const argument = m_arguments.back();
		m_arguments = m_arguments.subspan(0, m_arguments.size() - 1);
		if (!parse_positional(positional, argument.lexeme)) {
			return Result::create_early_exit(ExitCode::InvalidArgument);
		}
	}

	return Result::create_continue();
}

auto ParameterParser::parse_list() -> Result {
	if (m_list) {
		while (!m_arguments.empty()) {
			auto const token = m_arguments.front();
			m_arguments = m_arguments.subspan(1);

			if (m_list->parse(token.lexeme)) {
				continue;
			}

			m_context.print_error(std::format("invalid {}: '{}'", m_list->name, token.lexeme));
			return Result::create_early_exit(ExitCode::InvalidArgument);
		}
	}

	return Result::create_continue();
}

auto ParameterParser::check_required(parameter::Positional const& positional) -> Result {
	if (positional.type != parameter::Type::Required) {
		return Result::create_continue();
	}

	m_context.print_error(std::format("missing required argument: '{}'", positional.name));
	return Result::create_early_exit(ExitCode::MissingRequiredArgument);
}

auto ParameterParser::parse_positional(parameter::Positional const& positional, std::string_view const value)
	-> Result {
	if (positional.parse(value)) {
		return Result::create_continue();
	}

	m_context.print_error(std::format("invalid {}: '{}'", positional.name, value));
	return Result::create_early_exit(ExitCode::InvalidArgument);
}
} // namespace clap

#pragma endregion

#pragma region CommandParser

namespace clap {
auto CommandParser::set_spec(spec::Commands spec) -> Result {
	clear_spec();

	for (auto& command : spec.commands) {
		auto entry = Entry{
			.command = std::move(command),
			.parameter_parser = ParameterParser{m_context},
		};

		auto const result = entry.parameter_parser.set_parameters(std::move(entry.command.parameters));
		if (result.should_early_exit()) {
			clear_spec();
			return result;
		}

		m_entries.push_back(std::move(entry));
	}

	m_program_options = std::move(spec.options);
	m_policy = spec.policy;

	return Result::create_continue();
}

auto CommandParser::parse(std::span<std::string_view const> args) -> Result {
	m_args = {args.begin(), args.end()};
	m_command_token = {};
	m_selected_command = {};

	auto result = parse_program_options();
	if (result.should_early_exit()) {
		return result;
	}

	if (m_command_token.lexeme.empty()) {
		return Result::create_continue();
	}

	result = select_command();
	if (result.should_early_exit()) {
		return result;
	}

	result = parse_command_parameters();
	if (result.should_early_exit()) {
		return result;
	}

	if (!m_selected_command) {
		return Result::create_continue();
	}
	return Result{ExitCode::Success, Outcome::Continue, m_selected_command->command.identifier};
}

void CommandParser::clear_spec() {
	m_entries.clear();
	m_program_options.clear();
	m_policy = CommandPolicy::Required;
	m_args = {};
	m_command_token = {};
	m_selected_command = {};
}

auto CommandParser::parse_program_options() -> Result {
	auto option_parser = OptionParser{m_context, m_program_options};
	auto scanner = Scanner{m_args};
	auto token = Token{};
	while (scanner.scan_next(token)) {
		auto const action = option_parser.parse_next(token);

		if (auto const* result = std::get_if<Result>(&action); result && result->should_early_exit()) {
			return *result;
		}

		if (std::get<OptionParser::Iterate>(action) == OptionParser::Iterate::Pass) {
			m_command_token = token;
			break;
		}
	}

	if (auto const result = option_parser.finish(); result.should_early_exit()) {
		return result;
	}

	if (m_command_token.lexeme.empty() && m_policy == CommandPolicy::Required) {
		m_context.print_error("missing required command");
		return Result::create_early_exit(ExitCode::MissingRequiredCommand);
	}

	m_args = scanner.get_remain();
	return Result::create_continue();
}

auto CommandParser::select_command() -> Result {
	auto const pred = [this](Entry const& entry) {
		return entry.command.identifier == m_command_token.lexeme;
	};
	auto const it = std::ranges::find_if(m_entries, pred);
	if (it == m_entries.end()) {
		m_context.print_error(std::format("unrecognized command: {}", m_command_token.lexeme));
		return Result::create_early_exit(ExitCode::UnrecognizedCommand);
	}

	m_selected_command = &*it;
	return Result::create_continue();
}

auto CommandParser::parse_command_parameters() -> Result {
	if (!m_selected_command) { // CommandPolicy is Optional.
		if (!m_args.empty()) {
			m_context.print_error(std::format("unknown argument: {}", m_args.front()));
			return Result::create_early_exit(ExitCode::UnknownArgument);
		}

		return Result::create_continue();
	}

	// TODO: context:
	// - has_version() should return false
	// - should print selected command's help text
	return m_selected_command->parameter_parser.parse(m_args);
}
} // namespace clap

#pragma endregion
