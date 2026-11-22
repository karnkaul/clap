module;

#include <cassert>

module clap.detail;

#pragma region Context

namespace clap::detail {
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
} // namespace clap::detail

#pragma endregion

#pragma region Exception

namespace clap::detail {
ParseException::ParseException(std::string const& message, ExitCode const exit_code)
	: std::runtime_error{message},
	  m_exit_code(exit_code) {}
} // namespace clap::detail

#pragma endregion

#pragma region OptionParser

namespace clap::detail {
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

[[nodiscard]] auto or_iterate(Result const result, OptionParserOld::Iterate const it) -> OptionParserOld::Action {
	if (result.should_early_exit()) {
		return result;
	}
	return it;
}
} // namespace

OptionParserOld::OptionParserOld(IContext& context, std::span<parameter::Named const> options)
	: m_context(context),
	  m_options(options) {}

auto OptionParserOld::parse_next(Token const& token) -> Action {
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

auto OptionParserOld::finish() -> Result {
	if (!m_expects_value) {
		return Result::create_continue();
	}
	m_context.print_error(std::format("option '{}' requires an argument", m_option_lexeme));
	return Result::create_early_exit(ExitCode::OptionRequiresArgument);
}

auto OptionParserOld::parse_long(Token const& token) -> Action {
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

auto OptionParserOld::parse_shorts(Token const& token) -> Action {
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

auto OptionParserOld::parse_option_value(parameter::Named const& named,
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

auto OptionParserOld::assign_value_to(parameter::Named const& named,
									  std::string_view const value,
									  std::string_view const option_lexeme) const -> Result {
	if (!named.parse(value)) {
		m_context.print_error(std::format("invalid '{}': {}", option_lexeme, value));
		return Result::create_early_exit(ExitCode::InvalidArgument);
	}

	return Result::create_continue();
}

template <typename T>
auto OptionParserOld::find_named(T const t) const -> parameter::Named const* {
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

OptionParser::OptionParser(IContext& context, std::span<parameter::Named const> options)
	: m_context(context),
	  m_options(options) {}

auto OptionParser::parse_next(Token const& token) -> Iterate {
	if (m_force_ended) {
		return Iterate::Pass;
	}

	if (m_expects_value) {
		if (token.type == Token::Type::String) {
			assign_value_to(*m_expects_value, token.lexeme, m_option_lexeme);
			m_expects_value = {};
			m_option_lexeme = {};
			return Iterate::Consume;
		}

		auto const message = std::format("option '{}' requires an argument", m_option_lexeme);
		m_context.print_error(message);
		throw ParseException{message, ExitCode::OptionRequiresArgument};
	}

	switch (token.type) {
	case Token::Type::String: return Iterate::Pass;
	case Token::Type::MinusMinus: m_force_ended = true; return Iterate::Consume;
	case Token::Type::MinusMinusString: return parse_long(token);
	case Token::Type::MinusString: parse_shorts(token); return Iterate::Consume;
	default: break;
	}

	auto const mesage = std::format("InternalError: unexpected Token: '{}'", token.lexeme);
	m_context.print_error(mesage);
	throw ParseException{mesage, ExitCode::InternalError};
}

void OptionParser::finish() {
	if (!m_expects_value) {
		return;
	}

	auto const message = std::format("option '{}' requires an argument", m_option_lexeme);
	m_context.print_error(message);
	throw ParseException{message, ExitCode::OptionRequiresArgument};
}

auto OptionParser::parse_long(Token const& token) -> Iterate {
	auto const word = token.lexeme.substr(2);

	if (word == "version" && m_context.has_version()) {
		m_context.print_version();
		return Iterate::EarlyExit;
	}

	if (word == "help") {
		m_context.print_help_text();
		return Iterate::EarlyExit;
	}

	auto const split = ArgSplit::create(word);
	auto const& named = get_named(split.left);
	parse_option_value(named, split.right, token.lexeme);

	return Iterate::Consume;
}

void OptionParser::parse_shorts(Token const& token) {
	auto const word = token.lexeme.substr(1);
	auto const split = ArgSplit::create(word);
	auto letters = split.left;
	for (; letters.size() > 1; letters.remove_prefix(1)) {
		auto const& named = get_named(letters.front());
		if (!named.is_flag) {
			auto const message = std::format("option requires an argument -- '{}'", named.letter);
			m_context.print_error(message);
			throw ParseException{message, ExitCode::OptionRequiresArgument};
		}

		named.parse("true");
	}

	auto const& named = get_named(letters.front());
	parse_option_value(named, split.right, token.lexeme);
}

void OptionParser::parse_option_value(parameter::Named const& named,
									  std::string_view const value,
									  std::string_view const option_lexeme) {
	if (!named.is_flag && value.empty()) {
		m_expects_value = &named;
		m_option_lexeme = option_lexeme;
		return;
	}

	if (!value.empty()) {
		assign_value_to(named, value, option_lexeme);
		return;
	}

	if (!named.is_flag) {
		auto const message = std::format("option '{}' requires an argument", option_lexeme);
		m_context.print_error(message);
		throw ParseException{message, ExitCode::OptionRequiresArgument};
	}

	named.parse("true");
}

void OptionParser::assign_value_to(parameter::Named const& named,
								   std::string_view const value,
								   std::string_view const option_lexeme) const {
	if (!named.parse(value)) {
		auto const message = std::format("invalid '{}': {}", option_lexeme, value);
		m_context.print_error(message);
		throw ParseException{message, ExitCode::InvalidArgument};
	}
}

template <typename T>
auto OptionParser::get_named(T const t) const -> parameter::Named const& {
	auto const pred = [t](parameter::Named const& param) {
		if constexpr (std::same_as<T, char>) {
			return param.letter == t;
		} else {
			return param.word == t;
		}
	};
	auto const it = std::ranges::find_if(m_options, pred);
	if (it != m_options.end()) {
		return *it;
	}

	auto const message = std::format("unrecognized option: '{}'", t);
	m_context.print_error(message);
	throw ParseException{message, ExitCode::UnrecognizedOption};
}
} // namespace clap::detail

#pragma endregion

#pragma region environment

namespace clap::detail {
ParameterEnvironment::ParameterEnvironment(ErrorPrinter const& printer, ParameterList parameters) {
	options.clear();
	arguments.pre_list.clear();
	arguments.list.reset();
	arguments.post_list.clear();

	auto optional_positionals = false;
	auto const visitor = Visitor{
		[&](parameter::Named named) {
			options.push_back(std::move(named));
		},
		[&](parameter::Positional positional) {
			if (optional_positionals && positional.type == parameter::Type::Required) {
				auto const message = std::format("required positional after optional(s): '{}'", positional.name);
				printer.print_error(message);
				throw InvalidParameterException{message};
			}

			auto& target = arguments.list ? arguments.post_list : arguments.pre_list;
			target.push_back(std::move(positional));
			if (positional.type == parameter::Type::Optional) {
				optional_positionals = true;
			}
		},
		[&](parameter::List list) {
			if (arguments.list) {
				auto const message = std::format("only one list parameter is permitted: '{}'", list.name);
				printer.print_error(message);
				throw InvalidParameterException{message};
			}

			arguments.list.emplace(std::move(list));
		},
	};

	for (auto& parameter : parameters) {
		std::visit(visitor, std::move(parameter));
	}
}

CommandEnvironment::CommandEnvironment(ErrorPrinter const& printer, Command command)
	: identifier(command.identifier),
	  description(command.description),
	  epilogue(command.epilogue),
	  parameter_environment(printer, std::move(command.parameters)) {}

ProgramEnvironment::ProgramEnvironment(ErrorPrinter const& printer,
									   Program const& program,
									   std::variant<ParameterList, OptionList> program_parameters,
									   CommandList commands,
									   CommandPolicy const command_policy)
	: program_name(program.name),
	  version(program.version),
	  description(program.description),
	  command_policy(command_policy) {
	auto const visitor = Visitor{
		[&](ParameterList parameter_list) {
			parameters = ParameterEnvironment{printer, std::move(parameter_list)};
		},
		[&](OptionList option_list) {
			parameters.options = std::move(option_list);
		},
	};

	std::visit(visitor, std::move(program_parameters));

	this->commands.clear();
	for (auto& command : commands) {
		this->commands.push_back(CommandEnvironment{printer, std::move(command)});
	}
}
} // namespace clap::detail

#pragma endregion

#pragma region ParameterParser

namespace clap::detail {
ParameterParser::ParameterParser(IContext& context, ParameterEnvironment const& environment)
	: m_context(context),
	  m_environment(environment) {}

auto ParameterParser::parse(Scanner& scanner) -> Result {
	m_argument_tokens.clear();
	m_arguments = {};

	auto option_parser = OptionParser{m_context, m_environment.options};
	auto token = Token{};

	try {
		while (scanner.scan_next(token)) {
			auto const action = option_parser.parse_next(token);
			if (action == Iterate::Pass) {
				m_argument_tokens.push_back(token);
			}
		}

		option_parser.finish();

		parse_arguments();

		return Result::create_continue();
	} catch (ParseException const& e) {
		return Result{e.get_exit_code(), Outcome::EarlyExit, {}};
	}
}

void ParameterParser::parse_arguments() {
	m_arguments = m_argument_tokens;
	parse_front_to_list();
	parse_back_to_list();
	parse_list();

	if (m_arguments.empty()) {
		return;
	}

	auto const message = std::format("unknown argument: '{}'", m_arguments.front().lexeme);
	m_context.print_error(message);
	throw ParseException{message, ExitCode::UnknownArgument};
}

void ParameterParser::parse_front_to_list() {
	for (auto const& positional : m_environment.arguments.pre_list) {
		if (m_arguments.empty()) {
			check_required(positional);
			return;
		}

		auto const argument = m_arguments.front();
		m_arguments = m_arguments.subspan(1);
		parse_positional(positional, argument.lexeme);
	}
}

void ParameterParser::parse_back_to_list() {
	for (auto const& positional : std::views::reverse(m_environment.arguments.post_list)) {
		if (m_arguments.empty()) {
			check_required(positional);
			return;
		}

		auto const argument = m_arguments.back();
		m_arguments = m_arguments.subspan(0, m_arguments.size() - 1);
		parse_positional(positional, argument.lexeme);
	}
}

void ParameterParser::parse_list() {
	if (m_environment.arguments.list) {
		while (!m_arguments.empty()) {
			auto const token = m_arguments.front();
			m_arguments = m_arguments.subspan(1);

			if (m_environment.arguments.list->parse(token.lexeme)) {
				continue;
			}

			auto const message = std::format("invalid {}: '{}'", m_environment.arguments.list->name, token.lexeme);
			m_context.print_error(message);
			throw ParseException{message, ExitCode::InvalidArgument};
		}
	}
}

void ParameterParser::check_required(parameter::Positional const& positional) {
	if (positional.type != parameter::Type::Required) {
		return;
	}

	auto const message = std::format("missing required argument: '{}'", positional.name);
	m_context.print_error(message);
	throw ParseException{message, ExitCode::MissingRequiredArgument};
}

void ParameterParser::parse_positional(parameter::Positional const& positional, std::string_view const value) {
	if (positional.parse(value)) {
		return;
	}

	auto const message = std::format("invalid {}: '{}'", positional.name, value);
	m_context.print_error(message);
	throw ParseException{message, ExitCode::InvalidArgument};
}
} // namespace clap::detail

#pragma endregion

#pragma region ProgramParser

namespace clap::detail {
ProgramParser::ProgramParser(IContext& context, ProgramEnvironment const& environment)
	: m_context(context),
	  m_environment(environment) {}

auto ProgramParser::parse(std::span<std::string_view const> args) -> Result {
	auto scanner = Scanner{args};
	try {
		if (m_environment.commands.empty()) {
			return parse_parameters(scanner);
		}
		return parse_commands(scanner);
	} catch (ParseException const& e) {
		return Result{e.get_exit_code(), Outcome::EarlyExit, {}};
	}
}

auto ProgramParser::parse_parameters(Scanner& scanner) -> Result {
	auto parameter_parser = ParameterParser{m_context, m_environment.parameters};
	return parameter_parser.parse(scanner);
}

auto ProgramParser::parse_commands(Scanner& scanner) -> Result {
	m_command_token = {};
	m_selected_command = {};

	parse_program_options(scanner);

	select_command();
	parse_command_parameters(scanner);

	if (!m_selected_command) {
		return Result::create_continue();
	}

	return Result{ExitCode::Success, Outcome::Continue, m_selected_command->identifier};
}

void ProgramParser::parse_program_options(Scanner& scanner) {
	auto option_parser = OptionParser{m_context, m_environment.parameters.options};
	auto token = Token{};
	while (scanner.scan_next(token)) {
		auto const action = option_parser.parse_next(token);

		if (action == Iterate::Pass) {
			m_command_token = token;
			break;
		}
	}

	option_parser.finish();
}

void ProgramParser::select_command() {
	if (m_command_token.lexeme.empty()) {
		if (m_environment.command_policy == CommandPolicy::Required) {
			auto const message = std::string{"missing required command"};
			m_context.print_error(message);
			throw ParseException{message, ExitCode::MissingRequiredCommand};
		}

		return;
	}

	auto const pred = [this](CommandEnvironment const& entry) {
		return entry.identifier == m_command_token.lexeme;
	};
	auto const it = std::ranges::find_if(m_environment.commands, pred);
	if (it == m_environment.commands.end()) {
		auto const message = std::format("unrecognized command: {}", m_command_token.lexeme);
		m_context.print_error(message);
		throw ParseException{message, ExitCode::UnrecognizedCommand};
	}

	m_selected_command = &*it;
}

void ProgramParser::parse_command_parameters(Scanner& scanner) {
	if (!m_selected_command) { // CommandPolicy is Optional.
		auto const environment = ParameterEnvironment{};
		ParameterParser{m_context, environment}.parse(scanner);
		return;
	}

	// TODO: context:
	// - has_version() should return false
	// - should print selected command's help text
	auto parameter_parser = ParameterParser{m_context, m_selected_command->parameter_environment};
	parameter_parser.parse(scanner);
}
} // namespace clap::detail

#pragma endregion
