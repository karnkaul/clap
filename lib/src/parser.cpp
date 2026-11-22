module;

#include <cassert>

module clap.parser;

#pragma region Context

namespace clap {
void ErrorPrinter::print_error(std::string_view const message) const {
	if (!printer) {
		return;
	}

	auto text = std::string{};
	if (!program_name.empty()) {
		std::format_to(std::back_inserter(text), "{}: ", program_name);
	}
	if (!command_identifier.empty()) {
		std::format_to(std::back_inserter(text), "{}: ", command_identifier);
	}

	text.append(message);
	printer->printerr(text);
}

ProgramContext::ProgramContext(Program const& program, IPrinter* custom_printer)
	: m_error_printer{.printer = &or_default(custom_printer), .program_name = program.name},
	  m_version(program.version),
	  m_description(program.description),
	  m_printer(m_error_printer.printer) {}

void ProgramContext::print_help_text() {
	// TODO
}

auto ProgramContext::has_version() const -> bool { return !m_version.empty(); }

void ProgramContext::print_version() {
	if (m_version.empty()) {
		return;
	}

	m_printer->println(m_version);
}

void ProgramContext::set_command_identifier(std::string_view const identifier) {
	m_command_identifier = identifier;
	m_version = {};
}
} // namespace clap

#pragma endregion

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

#pragma region environment

namespace clap {
auto ParameterEnvironment::build(ErrorPrinter const& printer, ParameterList parameters) -> ExitCode {
	options.clear();
	arguments.pre_list.clear();
	arguments.list.reset();
	arguments.post_list.clear();

	auto optional_positionals = false;
	auto const visitor = Visitor{
		[&](parameter::Named named) {
			options.push_back(std::move(named));
			return ExitCode::Success;
		},
		[&](parameter::Positional positional) {
			if (optional_positionals && positional.type == parameter::Type::Required) {
				printer.print_error(std::format("required positional after optional(s): '{}'", positional.name));
				return ExitCode::InvalidParameter;
			}

			auto& target = arguments.list ? arguments.post_list : arguments.pre_list;
			target.push_back(std::move(positional));
			if (positional.type == parameter::Type::Optional) {
				optional_positionals = true;
			}

			return ExitCode::Success;
		},
		[&](parameter::List list) {
			if (arguments.list) {
				printer.print_error(std::format("only one list parameter is permitted: '{}'", list.name));
				return ExitCode::InvalidParameter;
			}

			arguments.list.emplace(std::move(list));
			return ExitCode::Success;
		},
	};

	for (auto& parameter : parameters) {
		auto const result = std::visit(visitor, std::move(parameter));
		if (result != ExitCode::Success) {
			return result;
		}
	}

	return ExitCode::Success;
}

auto CommandEnvironment::build(ErrorPrinter const& printer, Command command) -> ExitCode {
	identifier = command.identifier;
	description = command.description;
	epilogue = command.epilogue;
	return parameter_environment.build(printer, std::move(command.parameters));
}

auto ProgramEnvironment::build(ErrorPrinter const& printer,
							   Program const& program,
							   std::variant<ParameterList, OptionList> program_parameters,
							   CommandList commands,
							   CommandPolicy const command_policy) -> ExitCode {
	program_name = program.name;
	version = program.version;
	description = program.description;
	this->command_policy = command_policy;

	auto const visitor = Visitor{
		[&](ParameterList parameter_list) {
			return parameters.build(printer, std::move(parameter_list));
		},
		[&](OptionList option_list) {
			parameters.options = std::move(option_list);
			return ExitCode::Success;
		},
	};

	auto const result = std::visit(visitor, std::move(program_parameters));
	if (result != ExitCode::Success) {
		return result;
	}

	this->commands.clear();
	for (auto& command : commands) {
		auto& environment = this->commands.emplace_back();
		auto const result = environment.build(printer, std::move(command));
		if (result != ExitCode::Success) {
			return result;
		}
	}

	return ExitCode::Success;
}
} // namespace clap

#pragma endregion

#pragma region ParameterParser

namespace clap {
ParameterParser::ParameterParser(IContext& context, ParameterEnvironment const& environment)
	: m_context(context),
	  m_environment(environment) {}

auto ParameterParser::parse(Scanner& scanner) -> Result {
	m_argument_tokens.clear();
	m_arguments = {};

	auto option_parser = OptionParser{m_context, m_environment.options};
	auto token = Token{};

	auto result = Result::create_continue();
	while (scanner.scan_next(token)) {
		auto action = option_parser.parse_next(token);

		auto const visitor = Visitor{
			[&](Iterate const it) {
				if (it == Iterate::Pass) {
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
	for (auto const& positional : m_environment.arguments.pre_list) {
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
	for (auto const& positional : std::views::reverse(m_environment.arguments.post_list)) {
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
	if (m_environment.arguments.list) {
		while (!m_arguments.empty()) {
			auto const token = m_arguments.front();
			m_arguments = m_arguments.subspan(1);

			if (m_environment.arguments.list->parse(token.lexeme)) {
				continue;
			}

			m_context.print_error(std::format("invalid {}: '{}'", m_environment.arguments.list->name, token.lexeme));
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

#pragma region ProgramParser

namespace clap {
ProgramParser::ProgramParser(IContext& context, ProgramEnvironment const& environment)
	: m_context(context),
	  m_environment(environment) {}

auto ProgramParser::parse(std::span<std::string_view const> args) -> Result {
	auto scanner = Scanner{args};
	if (m_environment.commands.empty()) {
		return parse_parameters(scanner);
	}
	return parse_commands(scanner);
}

auto ProgramParser::parse_parameters(Scanner& scanner) -> Result {
	auto parameter_parser = ParameterParser{m_context, m_environment.parameters};
	return parameter_parser.parse(scanner);
}

auto ProgramParser::parse_commands(Scanner& scanner) -> Result {
	m_command_token = {};
	m_selected_command = {};

	auto result = parse_program_options(scanner);
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

	result = parse_command_parameters(scanner);
	if (result.should_early_exit()) {
		return result;
	}

	if (!m_selected_command) {
		return Result::create_continue();
	}
	return Result{ExitCode::Success, Outcome::Continue, m_selected_command->identifier};
}

auto ProgramParser::parse_program_options(Scanner& scanner) -> Result {
	auto option_parser = OptionParser{m_context, m_environment.parameters.options};
	auto token = Token{};
	while (scanner.scan_next(token)) {
		auto const action = option_parser.parse_next(token);

		if (auto const* result = std::get_if<Result>(&action); result && result->should_early_exit()) {
			return *result;
		}

		if (std::get<Iterate>(action) == Iterate::Pass) {
			m_command_token = token;
			break;
		}
	}

	if (auto const result = option_parser.finish(); result.should_early_exit()) {
		return result;
	}

	if (m_command_token.lexeme.empty() && m_environment.command_policy == CommandPolicy::Required) {
		m_context.print_error("missing required command");
		return Result::create_early_exit(ExitCode::MissingRequiredCommand);
	}

	return Result::create_continue();
}

auto ProgramParser::select_command() -> Result {
	auto const pred = [this](CommandEnvironment const& entry) {
		return entry.identifier == m_command_token.lexeme;
	};
	auto const it = std::ranges::find_if(m_environment.commands, pred);
	if (it == m_environment.commands.end()) {
		m_context.print_error(std::format("unrecognized command: {}", m_command_token.lexeme));
		return Result::create_early_exit(ExitCode::UnrecognizedCommand);
	}

	m_selected_command = &*it;
	return Result::create_continue();
}

auto ProgramParser::parse_command_parameters(Scanner& scanner) -> Result {
	if (!m_selected_command) { // CommandPolicy is Optional.
		auto const environment = ParameterEnvironment{};
		return ParameterParser{m_context, environment}.parse(scanner);
	}

	// TODO: context:
	// - has_version() should return false
	// - should print selected command's help text
	auto parameter_parser = ParameterParser{m_context, m_selected_command->parameter_environment};
	return parameter_parser.parse(scanner);
}
} // namespace clap

#pragma endregion

#pragma region Parser

namespace clap {
Parser::Parser(spec::Parameters parameters) : m_context(parameters.program, parameters.custom_printer) {
	auto const result = m_environment.build(
		m_context.get_error_printer(), parameters.program, std::move(parameters.parameters), {}, {});
	if (result != ExitCode::Success) {
		// TODO
	}
}

Parser::Parser(spec::Commands commands) : m_context(commands.program, commands.custom_printer) {
	auto const result = m_environment.build(m_context.get_error_printer(),
											commands.program,
											std::move(commands.options),
											std::move(commands.commands),
											commands.policy);
	if (result != ExitCode::Success) {
		// TODO
	}
}

auto Parser::parse_args(std::span<std::string_view const> args) -> Result {
	auto program_parser = ProgramParser{m_context, m_environment};
	return program_parser.parse(args);
}
} // namespace clap
#pragma endregion
