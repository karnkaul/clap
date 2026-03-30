#include "clap/command.hpp"
#include "clap/exception.hpp"
#include "clap/parameter.hpp"
#include "clap/parser.hpp"
#include "clap/printer.hpp"
#include "clap/result.hpp"
#include "detail/parser_impl.hpp"
#include "detail/scanner.hpp"
#include "detail/types.hpp"
#include "detail/visitor.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <memory>
#include <print>
#include <ranges>
#include <string_view>
#include <utility>

namespace clap {
namespace fs = std::filesystem;

namespace {
[[nodiscard]] auto to_lower(char const in) {
	auto const ret = std::tolower(static_cast<unsigned char>(in));
	if (ret < 0) { return '\0'; }
	assert(ret <= int(std::numeric_limits<char>::max()));
	return char(ret);
}

[[nodiscard]] auto iequals(std::string_view const a, std::string_view const b) {
	if (a.size() != b.size()) { return false; }
	auto const pred = [](auto const& zip) {
		auto const [a, b] = zip;
		return to_lower(a) == to_lower(b);
	};
	return std::ranges::all_of(std::views::zip(a, b), pred);
}

struct Printer : IPrinter {
	[[nodiscard]] static auto self() -> Printer& {
		static auto ret = Printer{};
		return ret;
	}

	void println(std::string_view const message) final { std::println("{}", message); }
	void printerr(std::string_view const message) final { std::println(stderr, "{}", message); }
};
} // namespace

namespace detail {
Scanner::Scanner(std::span<std::string_view const> words) : m_remain(words) { advance(); }

auto Scanner::scan_next(Token& out) -> bool {
	if (m_current.empty()) {
		out = Token{.type = Token::Type::Eof};
		return false;
	}

	out = scan_next();
	return true;
}

auto Scanner::scan_next() -> Token {
	if (m_current == "--") { return to_token(Token::Type::MinusMinus, 2); }

	if (m_current.starts_with("--")) { return minus(Token::Type::MinusMinusString); }
	if (m_current.starts_with('-')) { return minus(Token::Type::MinusString); }

	if (m_current.starts_with('=')) { return to_token(Token::Type::Equals, 1); }

	return scan_string();
}

auto Scanner::minus(Token::Type const type) -> Token {
	auto const length = std::min(m_current.find('='), m_current.size());
	return to_token(type, length);
}

auto Scanner::scan_string() -> Token {
	auto const length = std::min(m_current.find('='), m_current.size());
	return to_token(Token::Type::String, length);
}

void Scanner::advance() {
	if (m_remain.empty()) {
		m_current = {};
		return;
	}
	m_current = m_remain.front();
	m_remain = m_remain.subspan(1);
}

auto Scanner::to_token(Token::Type const type, std::size_t const length) -> Token {
	auto const ret = Token{.type = type, .lexeme = m_current.substr(0, length)};
	m_current.remove_prefix(length);
	if (m_current.empty()) { advance(); }
	return ret;
}

PrinterWrapper::PrinterWrapper(Ptr<IPrinter> printer, std::string_view program_name) noexcept(true)
	: printer(printer ? printer : &IPrinter::default_printer()), program_name(program_name) {}

Frame::Frame(std::span<Parameter const> parameters) noexcept(false) {
	named_parameters.reserve(parameters.size());
	positional_parameters.reserve(parameters.size());

	auto positionals_ended = false;
	auto const check_extraneous = [&](auto const& t) {
		if (!positionals_ended) { return; }
		throw InvalidParameterException{std::format("extraneous positional: '{}'", t.name)};
	};

	auto const visitor = Visitor{
		[&](parameter::Named const& n) { named_parameters.push_back(&n); },
		[&](parameter::Positional const& p) {
			check_extraneous(p);
			if (p.type == parameter::Type::Optional) { positionals_ended = true; }
			positional_parameters.push_back(&p);
		},
		[&](parameter::List const& l) {
			check_extraneous(l);
			list_parameter = &l;
			positionals_ended = true;
		},
	};

	for (auto const& parameter : parameters) { std::visit(visitor, parameter); }
}

Frame::Frame(std::span<parameter::Named const> options) noexcept(true) {
	named_parameters.reserve(options.size());
	for (auto const& parameter : options) { named_parameters.push_back(&parameter); }
}

Context::Context(ParameterInput const& input) noexcept(false)
	: printer(input.printer, input.program.name), main(input.parameters), version(input.program.version), description(input.program.description) {}

Context::Context(CommandInput const& input) noexcept(false)
	: printer(input.printer, input.program.name), main(input.options), version(input.program.version), description(input.program.description),
	  command_policy(input.command_policy) {
	commands.reserve(input.commands.size());
	for (auto const& command : input.commands) {
		auto frame = Frame{command.parameters};
		frame.command = &command;
		commands.push_back(std::move(frame));
	}
}

namespace {
struct Joiner {
	void join(std::string_view const s) { join("{}", s); }

	template <typename... Args>
	void join(std::format_string<Args...> fmt, Args&&... args) {
		if (!text.empty() && text.back() != '\n' && !text.ends_with(delimiter)) { text.append(delimiter); }
		std::format_to(std::back_inserter(text), fmt, std::forward<Args>(args)...);
	}

	std::string_view delimiter{" "};

	std::string text{};
};

void serialize_positionals_usage(Joiner& joiner, std::span<Ptr<parameter::Positional const> const> positionals, Ptr<parameter::List const> list) {
	for (auto const& positional : positionals) {
		if (positional->type == parameter::Type::Optional) {
			joiner.join("[{}]", positional->name);
		} else {
			joiner.join("<{}>", positional->name);
		}
	}
	if (list) { joiner.join("[{}]...", list->name); }
	joiner.text.append("\n");
}

void serialize_named_list(Joiner& out, std::span<Ptr<parameter::Named const> const> parameters, bool const add_version) {
	out.text.append("\nOPTIONS\n");
	for (auto const* named : parameters) {
		out.text.append("  ");
		if (named->letter) {
			std::format_to(std::back_inserter(out.text), "-{}", named->letter);
			if (!named->word.empty()) { out.text.append(", "); }
		} else {
			out.text.append("    ");
		}
		if (!named->word.empty()) { std::format_to(std::back_inserter(out.text), "--{}", named->word); }
		out.text.append("\n");
		if (!named->description.empty()) { out.join("{: >8}{}\n", ' ', named->description); }
	}
	if (add_version) { std::format_to(std::back_inserter(out.text), "{: >6}--version\n{: >8}display version and exit\n", ' ', ' '); }
	std::format_to(std::back_inserter(out.text), "{: >6}--help\n{: >8}display this help and exit\n", ' ', ' ');
}

void serialize_positional_list(Joiner& out, std::span<Ptr<parameter::Positional const> const> parameters, Ptr<parameter::List const> list) {
	if (parameters.empty() && !list) { return; }
	out.text.append("\nARGUMENTS\n");
	for (auto const* positional : parameters) {
		out.join("  {}\n", positional->name);
		if (!positional->description.empty()) { out.join("{: >8}{}\n", ' ', positional->description); }
	}
	if (list) {
		out.join("  {}\n", list->name);
		if (!list->description.empty()) { out.join("{: >8}{}\n", ' ', list->description); }
	}
}

void serialize_command_list(Joiner& out, std::span<Frame const> commands) {
	if (commands.empty()) { return; }
	out.text.append("\nCOMMANDS\n");
	for (auto const& frame : commands) {
		out.join("  {}\n", frame.command->identifier);
		if (!frame.command->description.empty()) { out.join("{: >8}{}\n", ' ', frame.command->description); }
	}
}
} // namespace

ParserImpl::Environment::Environment(Context const& context) noexcept(false)
	: printer(context.printer), commands(context.commands), version(context.version), description(context.description), command_policy(context.command_policy),
	  frame(&context.main) {}

auto ParserImpl::Environment::next_positional() -> Ptr<parameter::Positional const> {
	if (positional_index >= frame->positional_parameters.size()) { return nullptr; }
	return frame->positional_parameters[positional_index++];
}

auto ParserImpl::Environment::set_command_frame(std::string_view const identifier) -> bool {
	auto const it = std::ranges::find_if(commands, [identifier](Frame const& f) { return f.command->identifier == identifier; });
	if (it == commands.end()) { return false; }

	frame = &*it;
	positional_index = 0uz;
	return true;
}

auto ParserImpl::Environment::help_text_main() const -> std::string {
	auto joiner = Joiner{};

	joiner.text = "Usage:";
	joiner.join(printer.program_name);
	joiner.join("[OPTION]...");
	serialize_positionals_usage(joiner, frame->positional_parameters, frame->list_parameter);

	if (!description.empty()) { joiner.join("{}\n", description); }

	serialize_named_list(joiner, frame->named_parameters, !version.empty());
	serialize_positional_list(joiner, frame->positional_parameters, frame->list_parameter);

	return std::move(joiner.text);
}

auto ParserImpl::Environment::help_text_commands() const -> std::string {
	auto joiner = Joiner{};

	joiner.text = "Usage:";
	joiner.join(printer.program_name);
	joiner.join("[OPTION]...");
	if (command_policy == CommandPolicy::Optional) {
		joiner.join("[COMMAND]");
	} else {
		joiner.join("<COMMAND>");
	}
	joiner.join("[OPTION]... <ARG>...\n");

	if (!description.empty()) { joiner.join("{}\n", description); }

	serialize_named_list(joiner, frame->named_parameters, !version.empty());
	serialize_command_list(joiner, commands);

	return std::move(joiner.text);
}

auto ParserImpl::Environment::help_text_command() const -> std::string {
	assert(frame->command);

	auto joiner = Joiner{};

	joiner.text = "Usage:";
	joiner.join(printer.program_name);
	joiner.join("{}", frame->command->identifier);
	joiner.join("[OPTION]...");
	serialize_positionals_usage(joiner, frame->positional_parameters, frame->list_parameter);

	if (!frame->command->description.empty()) { joiner.join("{}\n", frame->command->description); }

	serialize_named_list(joiner, frame->named_parameters, false);
	serialize_positional_list(joiner, frame->positional_parameters, frame->list_parameter);

	return std::move(joiner.text);
}

auto ParserImpl::Environment::help_text() const -> std::string {
	if (frame->command) { return help_text_command(); }
	if (!commands.empty()) { return help_text_commands(); }
	return help_text_main();
}

auto ParserImpl::Environment::is_missing_required_command() const -> bool {
	return !commands.empty() && !frame->command && command_policy == CommandPolicy::Required;
}

ParserImpl::ParserImpl(Context const& context, std::span<std::string_view const> args) noexcept(true) : m_environment(context), m_scanner(args) {}

auto ParserImpl::parse() -> Result {
	advance();
	while (m_current.type != Token::Type::Eof && m_outcome == Outcome::Continue) { parse_current(); }
	auto ret = Result{.outcome = m_outcome};
	if (m_environment.frame->command) { ret.command_identifier = m_environment.frame->command->identifier; }
	if (m_outcome != Outcome::EarlyExit) {
		check_required_parsed();
		if (m_environment.is_missing_required_command()) {
			m_environment.printer.prefixed_err("missing required command");
			throw Error{Error::MissingRequiredCommand};
		}
	}
	return ret;
}

void ParserImpl::advance() { m_scanner.scan_next(m_current); }

void ParserImpl::parse_current() {
	if (m_options_terminated) {
		parse_argument();
		return;
	}

	switch (m_current.type) {
	case Token::Type::MinusMinus:
		m_options_terminated = true;
		advance();
		break;
	case Token::Type::String: parse_argument(); break;
	case Token::Type::MinusMinusString: parse_long_option(); break;
	case Token::Type::MinusString: parse_short_options(); break;

	case Token::Type::Eof:
	case Token::Type::Equals: m_environment.printer.prefixed_err("unexpected token: '{}'", m_current.lexeme); throw Error{Error::UnexpectedToken};

	default: m_environment.printer.prefixed_err("unrecognized token: '{}'", m_current.lexeme); throw Error{Error::UnrecognizedToken};
	}
}

void ParserImpl::parse_argument() {
	if (select_command()) { return; }

	auto const* positional = m_environment.next_positional();
	if (!positional) {
		if (!m_environment.frame->list_parameter) {
			m_environment.printer.prefixed_err("unknown argument: '{}'", m_current.lexeme);
			throw Error{Error::UnknownArgument};
		}
		if (!m_environment.frame->list_parameter->parse(m_current.lexeme)) {
			m_environment.printer.prefixed_err("invalid {}: '{}'", m_environment.frame->list_parameter->name, m_current.lexeme);
			throw Error{Error::InvalidArgument};
		}
		advance();
		return;
	}

	if (!positional->parse(m_current.lexeme)) {
		m_environment.printer.prefixed_err("invalid {}: '{}'", positional->name, m_current.lexeme);
		throw Error{Error::InvalidArgument};
	}
	advance();
}

void ParserImpl::parse_long_option() {
	assert(m_current.lexeme.starts_with("--") && m_current.lexeme.size() > 2);

	auto const option_token = m_current;
	auto const word = option_token.lexeme.substr(2);

	if (!m_environment.frame->command && !m_environment.version.empty() && word == "version") {
		m_environment.printer->println(m_environment.version);
		m_outcome = Outcome::EarlyExit;
		return;
	}

	if (word == "help") {
		m_environment.printer->println(help_text());
		m_outcome = Outcome::EarlyExit;
		return;
	}

	auto const& named = get_named(word);
	advance();
	parse_last_option(named, option_token.lexeme);
}

void ParserImpl::parse_short_options() {
	assert(m_current.lexeme.front() == '-' && m_current.lexeme.size() > 1);

	auto letters = m_current.lexeme.substr(1);
	for (; letters.size() > 1; letters.remove_prefix(1)) {
		auto const& named = get_named(letters.front());
		if (!named.is_flag) {
			m_environment.printer.prefixed_err("option requires an argument -- '{}'", named.letter);
			throw Error{Error::OptionRequiresArgument};
		}
		named.parse("true");
	}

	auto const& named = get_named(letters.front());
	advance();
	parse_last_option(named, letters);
}

void ParserImpl::parse_last_option(parameter::Named const& named, std::string_view const option_lexeme) {
	if (m_current.type == Token::Type::Equals) {
		advance();
		parse_option_value(named, option_lexeme);
		return;
	}

	if (named.is_flag) {
		named.parse("true");
		return;
	}

	parse_option_value(named, option_lexeme);
}

void ParserImpl::parse_option_value(parameter::Named const& named, std::string_view const option_lexeme) {
	if (m_current.type != Token::Type::String) {
		m_environment.printer.prefixed_err("option '{}' requires an argument", option_lexeme);
		throw Error{Error::OptionRequiresArgument};
	}

	if (!named.parse(m_current.lexeme)) {
		m_environment.printer.prefixed_err("invalid '{}': {}", named.word, m_current.lexeme);
		throw Error{Error::InvalidArgument};
	}

	advance();
}

template <typename T>
auto ParserImpl::get_named(T const t) const -> parameter::Named const& {
	auto const pred = [t](Ptr<parameter::Named const> n) {
		if constexpr (std::same_as<T, char>) {
			return n->letter == t;
		} else {
			return n->word == t;
		}
	};
	auto const it = std::ranges::find_if(m_environment.frame->named_parameters, pred);
	if (it != m_environment.frame->named_parameters.end()) { return **it; }
	m_environment.printer.prefixed_err("unrecognized option: '{}'", m_current.lexeme);
	throw Error{Error::UnrecognizedOption};
}

auto ParserImpl::select_command() -> bool {
	if (m_environment.frame->command) { return false; }
	auto const ret = m_environment.set_command_frame(m_current.lexeme);
	if (ret) { advance(); }
	return ret;
}

void ParserImpl::check_required_parsed() {
	auto const* remaining = m_environment.next_positional();
	if (!remaining || remaining->type == parameter::Type::Optional) { return; }
	m_environment.printer.prefixed_err("missing required argument: '{}'", remaining->name);
	throw Error{Error::MissingRequiredArgument};
}
} // namespace detail

namespace parameter {
auto Parse::operator()(std::string_view const input) const -> bool {
	if (was_set) { *was_set = true; }
	if (!func(input)) { return false; }
	return true;
}
} // namespace parameter

auto IPrinter::default_printer() -> IPrinter& { return Printer::self(); }

namespace {
[[nodiscard]] auto to_parameter_input(ParameterList const& parameters, Program const& program, detail::Ptr<IPrinter> custom_printer) {
	return detail::ParameterInput{
		.parameters = parameters,
		.program = program,
		.printer = custom_printer ? custom_printer : &IPrinter::default_printer(),
	};
}

[[nodiscard]] auto to_command_input(OptionList const& options, std::span<Command const> commands, Program const& program,
									detail::Ptr<IPrinter> custom_printer) {
	return detail::CommandInput{
		.options = options,
		.commands = commands,
		.program = program,
		.printer = custom_printer ? custom_printer : &IPrinter::default_printer(),
	};
}

[[nodiscard]] constexpr auto to_exit_code(detail::Error const err) {
	switch (err) {
	case detail::Error::UnrecognizedToken: return ExitCode::InternalError;
	case detail::Error::UnknownArgument: return ExitCode::UnknownArgument;
	case detail::Error::InvalidArgument: return ExitCode::InvalidArgument;
	case detail::Error::UnrecognizedOption: return ExitCode::UnrecognizedOption;
	case detail::Error::OptionRequiresArgument: return ExitCode::OptionRequiresArgument;
	case detail::Error::MissingRequiredArgument: return ExitCode::MissingRequiredArgument;
	case detail::Error::MissingRequiredCommand: return ExitCode::MissingRequiredCommand;
	case detail::Error::UnexpectedToken: return ExitCode::UnexpectedToken;
	default: return ExitCode::Failure;
	}
}
} // namespace

void Parser::Deleter::operator()(detail::Context* ptr) const noexcept { std::default_delete<detail::Context>{}(ptr); }

Parser::Parser(ParameterList parameters, Program const& program, IPrinter* custom_printer)
	: m_parameters(std::move(parameters)), m_context(new detail::Context{to_parameter_input(m_parameters, program, custom_printer)}) {}

Parser::Parser(CommandList commands, OptionList options, Program const& program, IPrinter* custom_printer)
	: m_options(std::move(options)), m_commands(std::move(commands)),
	  m_context(new detail::Context{to_command_input(m_options, m_commands, program, custom_printer)}) {}

auto Parser::parse_words(std::span<std::string_view const> words) const -> Result {
	if (!m_context) { return Result::error(ExitCode::InvalidParser); }
	try {
		auto impl = detail::ParserImpl{*m_context, words};
		auto const result = impl.parse();
		return Result{ExitCode::Success, result.outcome, result.command_identifier};
	} catch (detail::Error const error) { return Result::error(to_exit_code(error)); }
}

auto Parser::parse_line(std::string_view const line) const -> Result {
	if (!m_context) { return Result::error(ExitCode::InvalidParser); }
	auto const words = to_words(line);
	return parse_words(words);
}

auto Parser::parse_main(int const argc, char const* const* argv, bool const skip_argv_0) const -> Result {
	if (!m_context) { return Result::error(ExitCode::InvalidParser); }
	auto span = std::span{argv, std::size_t(argc)};
	if (skip_argv_0 && !span.empty()) { span = span.subspan(1); }
	auto const words = std::vector<std::string_view>{span.begin(), span.end()};
	return parse_words(words);
}

auto Parser::get_help_text() const -> std::string { return detail::ParserImpl{*m_context, {}}.help_text(); }

void parameter::split_named_key(std::string_view const key, char& out_letter, std::string_view& out_word) {
	if (key.empty()) { return; }

	if (key.size() == 1) {
		out_letter = key.front();
		return;
	}

	auto const comma = key.find(',');
	if (comma == std::string_view::npos) {
		out_word = key;
		return;
	}

	out_letter = key.front();
	out_word = key.substr(comma + 1);
}

auto parameter::parse_to(bool& out, std::string_view const input) -> bool {
	if (iequals(input, "true")) {
		out = true;
		return true;
	}

	if (iequals(input, "false")) {
		out = false;
		return true;
	}

	auto num = int{};
	if (!parse_to(num, input)) { return false; }

	out = static_cast<bool>(num);
	return true;
}
} // namespace clap

namespace {
class WordScanner {
  public:
	explicit constexpr WordScanner(std::string_view const line) : m_remain(line) {}

	constexpr auto next(std::string_view& out) -> bool {
		trim_whitespace();
		if (m_remain.empty()) { return false; }
		if (m_remain.front() == '"') {
			out = scan_quoted();
			return true;
		}
		out = scan_unquoted();
		return true;
	}

  private:
	static constexpr auto is_whitespace(char const c) { return c == ' ' || c == '\t'; };

	constexpr void trim_whitespace() {
		while (!m_remain.empty() && is_whitespace(m_remain.front())) { m_remain.remove_prefix(1); }
	}

	constexpr auto scan_quoted() -> std::string_view {
		assert(!m_remain.empty() && m_remain.front() == '"');
		m_remain.remove_prefix(1);

		auto const i = m_remain.find('"');
		if (i == std::string_view::npos) { return std::exchange(m_remain, {}); }

		auto const ret = m_remain.substr(0, i);
		m_remain.remove_prefix(i + 1);
		return ret;
	}

	constexpr auto scan_unquoted() -> std::string_view {
		for (auto length = 1uz; length < m_remain.size(); ++length) {
			char const c = m_remain[length];
			if (is_whitespace(c) || c == '"') {
				auto const ret = m_remain.substr(0, length);
				m_remain.remove_prefix(length);
				return ret;
			}
		}
		return std::exchange(m_remain, {});
	}

	std::string_view m_remain{};
};
} // namespace

auto clap::to_program_name(std::string_view const argv_0) -> std::string { return fs::absolute(argv_0).stem().string(); }

auto clap::to_words(std::string_view const line) -> std::vector<std::string_view> {
	auto ret = std::vector<std::string_view>{};
	auto scanner = WordScanner{line};
	for (auto word = std::string_view{}; scanner.next(word);) { ret.push_back(word); }
	return ret;
}

auto clap::command(std::string_view const identifier, ParameterList parameters, std::string_view const description, std::string_view epilogue) -> Command {
	return Command{identifier, std::move(parameters), description, epilogue};
}
