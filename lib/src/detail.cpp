module;

#include <cassert>

module clap.detail;

#pragma region ErrorPrinter

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
} // namespace clap::detail

#pragma endregion

#pragma region Exception

namespace clap::detail {
ParseException::ParseException(std::string const& message, ExitCode const exit_code)
	: std::runtime_error{message},
	  m_exit_code(exit_code) {}
} // namespace clap::detail

#pragma endregion

#pragma region ProgramContext

namespace clap::detail {
namespace {
struct Joiner {
	void join(std::string_view const s) { join("{}", s); }

	template <typename... Args>
	void join(std::format_string<Args...> fmt, Args&&... args) {
		if (!text.empty() && text.back() != '\n' && !text.ends_with(delimiter)) {
			text.append(delimiter);
		}
		std::format_to(std::back_inserter(text), fmt, std::forward<Args>(args)...);
	}

	void serialize_positionals_usage(std::span<parameter::Positional const> positionals) {
		for (auto const& positional : positionals) {
			if (positional.type == parameter::Type::Optional) {
				join("[{}]", positional.name);
			} else {
				join("<{}>", positional.name);
			}
		}
	}

	void serialize_positionals_usage(ParameterEnvironment::Arguments const& arguments) {
		serialize_positionals_usage(arguments.pre_list);
		if (arguments.list) {
			join("[{}]...", arguments.list->name);
		}
		serialize_positionals_usage(arguments.post_list);
		text.push_back('\n');
	}

	void serialize_named_list(std::span<parameter::Named const> parameters, bool const add_version) {
		text.append("\nOPTIONS\n");
		for (auto const& named : parameters) {
			text.append("  ");
			if (named.letter) {
				std::format_to(std::back_inserter(text), "-{}", named.letter);
				if (!named.word.empty()) {
					text.append(", ");
				}
			} else {
				text.append("    ");
			}
			if (!named.word.empty()) {
				std::format_to(std::back_inserter(text), "--{}", named.word);
			}
			text.append("\n");
			if (!named.description.empty()) {
				join("{: >8}{}\n", ' ', named.description);
			}
		}
		if (add_version) {
			std::format_to(std::back_inserter(text), "{: >6}--version\n{: >8}display version and exit\n", ' ', ' ');
		}
		std::format_to(std::back_inserter(text), "{: >6}--help\n{: >8}display this help and exit\n", ' ', ' ');
	}

	void serialize_positional_list(std::span<parameter::Positional const> positionals) {
		for (auto const& positional : positionals) {
			join("  {}\n", positional.name);
			if (!positional.description.empty()) {
				join("{: >8}{}\n", ' ', positional.description);
			}
		}
	}

	void serialize_positional_list(ParameterEnvironment::Arguments const& arguments) {
		if (arguments.pre_list.empty() && !arguments.list && arguments.post_list.empty()) {
			return;
		}

		text.append("\nARGUMENTS\n");
		serialize_positional_list(arguments.pre_list);
		if (arguments.list) {
			join("  {}\n", arguments.list->name);
			if (!arguments.list->description.empty()) {
				join("{: >8}{}\n", ' ', arguments.list->description);
			}
		}
		serialize_positional_list(arguments.post_list);
	}

	void serialize_command_list(std::span<CommandEnvironment const> commands) {
		if (commands.empty()) {
			return;
		}

		text.append("\nCOMMANDS\n");
		for (auto const& command : commands) {
			join("  {}\n", command.identifier);
			if (!command.description.empty()) {
				join("{: >8}{}\n", ' ', command.description);
			}
		}
	}

	std::string_view delimiter{" "};

	std::string text{};
};

template <typename T>
void reset_parses(T const& params) {
	for (auto& param : params) {
		if (param.parse.was_set) {
			*param.parse.was_set = false;
		}
	}
}

void reset_parses(ParameterEnvironment::Arguments& arguments) {
	reset_parses(arguments.pre_list);
	reset_parses(arguments.post_list);
	if (arguments.list) {
		reset_parses(std::span{&*arguments.list, 1});
	}
}
} // namespace

ProgramContext::ProgramContext(Program const& program,
							   std::variant<ParameterList, OptionList> program_parameters,
							   CommandList commands,
							   CommandPolicy command_policy,
							   IPrinter* custom_printer)
	: m_error_printer{.printer = &or_default(custom_printer), .program_name = program.name},
	  m_environment(m_error_printer, program, std::move(program_parameters), std::move(commands), command_policy),
	  m_printer(m_error_printer.printer) {}

void ProgramContext::print_help_text() { m_printer->println(help_text()); }

auto ProgramContext::has_version() const -> bool { return !m_selected_command && !m_environment.version.empty(); }

void ProgramContext::print_version() {
	if (!has_version()) {
		return;
	}

	m_printer->println(m_environment.version);
}

auto ProgramContext::help_text_parameters() const -> std::string {
	auto joiner = Joiner{};

	joiner.text = "Usage:";
	joiner.join(m_error_printer.program_name);
	if (m_selected_command) {
		joiner.join("{}", m_error_printer.command_identifier);
	}
	joiner.join("[OPTION]...");
	if (m_selected_command) {
		joiner.serialize_positionals_usage(m_selected_command->parameter_environment.arguments);
	} else {
		joiner.serialize_positionals_usage(m_environment.parameters.arguments);
	}

	auto const desc = m_selected_command ? m_selected_command->description : m_environment.description;
	if (!desc.empty()) {
		joiner.join("{}\n", desc);
	}

	if (m_selected_command) {
		joiner.serialize_named_list(m_selected_command->parameter_environment.options, false);
		joiner.serialize_positional_list(m_selected_command->parameter_environment.arguments);
	} else {
		joiner.serialize_named_list(m_environment.parameters.options, has_version());
		joiner.serialize_positional_list(m_environment.parameters.arguments);
	}

	return std::move(joiner.text);
}

auto ProgramContext::help_text_commands() const -> std::string {
	auto joiner = Joiner{};

	joiner.text = "Usage:";
	joiner.join(m_error_printer.program_name);
	joiner.join("[OPTION]...");
	joiner.join(m_environment.command_policy == CommandPolicy::Optional ? "[COMMAND]" : "<COMMAND>");
	joiner.join("[OPTION]... <ARG>...\n");

	if (!m_environment.description.empty()) {
		joiner.join("{}\n", m_environment.description);
	}

	joiner.serialize_named_list(m_environment.parameters.options, has_version());
	joiner.serialize_command_list(m_environment.commands);

	return std::move(joiner.text);
}

auto ProgramContext::help_text() const -> std::string {
	if (!m_selected_command && !m_environment.commands.empty()) {
		return help_text_commands();
	}
	return help_text_parameters();
}

void ProgramContext::fixup_program_name(std::string_view const arg0) {
	if (!m_environment.program_name.empty() || arg0.empty()) {
		return;
	}

	m_program_name = to_program_name(arg0);
	m_environment.program_name = m_program_name;
}

void ProgramContext::set_selected_command(CommandEnvironment const* command_enviroment) {
	m_selected_command = command_enviroment;
	if (m_selected_command) {
		m_error_printer.command_identifier = m_selected_command->identifier;
	}
}

void ProgramContext::reset_environment() {
	m_selected_command = nullptr;
	m_error_printer.command_identifier = {};
	m_environment.reset_parse_states();
}
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
} // namespace

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

void ParameterEnvironment::reset_parse_states() {
	reset_parses(arguments);
	reset_parses(options);
}

CommandEnvironment::CommandEnvironment(ErrorPrinter const& printer, Command command)
	: identifier(command.identifier),
	  description(command.description),
	  parameter_environment(printer, std::move(command.parameters)) {}

void CommandEnvironment::reset_parse_states() { parameter_environment.reset_parse_states(); }

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
		this->commands.emplace_back(printer, std::move(command));
	}
}

void ProgramEnvironment::reset_parse_states() {
	parameters.reset_parse_states();
	for (auto& command : commands) {
		command.reset_parse_states();
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

	while (scanner.scan_next(token)) {
		auto const action = option_parser.parse_next(token);
		switch (action) {
		case Iterate::Pass: m_argument_tokens.push_back(token); break;
		case Iterate::Consume: break;
		case Iterate::EarlyExit: return Result::create_early_exit(ExitCode::Success);
		}
	}

	option_parser.finish();

	parse_arguments();

	return Result::create_continue();
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
ProgramParser::ProgramParser(ProgramContext& context) : m_context(context) {}

auto ProgramParser::parse(std::span<std::string_view const> words) -> Result {
	auto scanner = Scanner{words};
	try {
		if (m_context.get_environment().commands.empty()) {
			return parse_parameters(scanner);
		}
		return parse_commands(scanner);
	} catch (ParseException const& e) {
		return Result{e.get_exit_code(), Outcome::EarlyExit, {}};
	}
}

auto ProgramParser::parse_parameters(Scanner& scanner) -> Result {
	auto parameter_parser = ParameterParser{m_context, m_context.get_environment().parameters};
	return parameter_parser.parse(scanner);
}

auto ProgramParser::parse_commands(Scanner& scanner) -> Result {
	m_command_token = {};
	m_selected_command = {};

	auto result = parse_program_options(scanner);
	if (result.should_early_exit()) {
		return result;
	}

	select_command();
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
	auto option_parser = OptionParser{m_context, m_context.get_environment().parameters.options};
	auto token = Token{};
	while (scanner.scan_next(token)) {
		auto const action = option_parser.parse_next(token);

		if (action == Iterate::Pass) {
			m_command_token = token;
			break;
		}

		if (action == Iterate::EarlyExit) {
			return Result::create_early_exit(ExitCode::Success);
		}
	}

	option_parser.finish();
	return Result::create_continue();
}

void ProgramParser::select_command() {
	if (m_command_token.lexeme.empty()) {
		if (m_context.get_environment().command_policy == CommandPolicy::Required) {
			auto const message = std::string{"missing required command"};
			m_context.print_error(message);
			throw ParseException{message, ExitCode::MissingRequiredCommand};
		}

		return;
	}

	auto const pred = [this](CommandEnvironment const& entry) {
		return entry.identifier == m_command_token.lexeme;
	};
	auto const it = std::ranges::find_if(m_context.get_environment().commands, pred);
	if (it == m_context.get_environment().commands.end()) {
		auto const message = std::format("unrecognized command: {}", m_command_token.lexeme);
		m_context.print_error(message);
		throw ParseException{message, ExitCode::UnrecognizedCommand};
	}

	m_selected_command = &*it;
	m_context.set_selected_command(m_selected_command);
}

auto ProgramParser::parse_command_parameters(Scanner& scanner) -> Result {
	if (!m_selected_command) {	  // CommandPolicy is Optional.
		assert(scanner.at_end()); // NOLINT(misc-static-assert)
		return Result::create_continue();
	}

	auto parameter_parser = ParameterParser{m_context, m_selected_command->parameter_environment};
	return parameter_parser.parse(scanner);
}
} // namespace clap::detail

#pragma endregion
