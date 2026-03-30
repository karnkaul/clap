#include "clap/parameter.hpp"
#include "clap/printer.hpp"
#include "detail/common.hpp"
#include "detail/parser.hpp"
#include "detail/scanner.hpp"
#include "detail/visitor.hpp"
#include <algorithm>
#include <cassert>
#include <limits>
#include <print>
#include <ranges>
#include <string_view>

namespace clap {
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
	auto const length = std::min(m_current.find_first_of("-="), m_current.size());
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

auto ParseFrame2::create(PrinterWrapper const& printer, std::span<Parameter const> parameters) noexcept(false) -> ParseFrame2 {
	auto ret = ParseFrame2{};
	ret.named_parameters.reserve(parameters.size());
	ret.positional_parameters.reserve(parameters.size());

	auto positionals_ended = false;
	auto const check_extraneous_positional = [&](auto const& positional) {
		if (!positionals_ended) { return; }
		printer.printerr_prefixed("extraneous positional: '{}'", positional.name);
		throw error::Parameter{error::Parameter::ExtraneousPositional};
	};

	auto const visitor = Visitor{
		[&](parameter::Named const& n) { ret.named_parameters.push_back(&n); },
		[&](parameter::Positional const& p) {
			check_extraneous_positional(p);
			if (p.type == parameter::Type::Optional) { positionals_ended = true; }
			ret.positional_parameters.push_back(&p);
		},
		[&](parameter::List const& l) {
			check_extraneous_positional(l);
			ret.list_parameter = &l;
			positionals_ended = true;
		},
	};

	for (auto const& parameter : parameters) { std::visit(visitor, parameter); }
	return ret;
}

auto ParseFrame2::create(PrinterWrapper const& printer, Command const& command) noexcept(false) -> ParseFrame2 {
	auto ret = create(printer, command.parameters);
	ret.command = &command;
	return ret;
}

void ParseFrame::recreate(PrinterWrapper const& printer, std::span<Parameter const> parameters) noexcept(false) {
	reset(parameters.size());

	auto positionals_ended = false;
	auto const check_extraneous_positional = [&](auto const& positional) {
		if (!positionals_ended) { return; }
		printer.printerr_prefixed("extraneous positional: '{}'", positional.name);
		throw error::Parameter{error::Parameter::ExtraneousPositional};
	};

	auto const visitor = Visitor{
		[&](parameter::Named const& n) { named_parameters.push_back(&n); },
		[&](parameter::Positional const& p) {
			check_extraneous_positional(p);
			if (p.type == parameter::Type::Optional) { positionals_ended = true; }
			positional_parameters.push_back(&p);
		},
		[&](parameter::List const& l) {
			check_extraneous_positional(l);
			list_parameter = &l;
			positionals_ended = true;
		},
	};

	for (auto const& parameter : parameters) { std::visit(visitor, parameter); }
}

void ParseFrame::recreate(PrinterWrapper const& printer, Command const& command) noexcept(false) {
	this->command = &command;
	recreate(printer, command.parameters);
}

void ParseFrame::reset(std::size_t const reserve) {
	named_parameters.clear();
	named_parameters.reserve(reserve);
	positional_parameters.clear();
	positional_parameters.reserve(reserve);
	list_parameter = nullptr;
}

auto ParseContext::create(ParseInput const& input) noexcept(false) -> ParseContext {
	auto ret = ParseContext{
		.printer = PrinterWrapper{.printer = input.printer, .program_name = input.program.name},
		.version = input.program.version,
		.description = input.program.description,
	};
	ret.main = ParseFrame2::create(ret.printer, input.parameters);
	ret.commands.reserve(input.commands.size());
	for (auto const& command : input.commands) { ret.commands.push_back(ParseFrame2::create(ret.printer, command)); }
	return ret;
}

Parser::Parser(ParseInput const& input, std::span<std::string_view const> args) : m_program(input.program), m_commands(input.commands), m_scanner(args) {
	m_printer = PrinterWrapper{
		.printer = input.printer ? input.printer : &IPrinter::default_printer(),
		.program_name = m_program.name,
	};
	m_frame.recreate(m_printer, input.parameters);
}

auto Parser::parse() -> Result {
	advance();
	while (m_current.type != Token::Type::Eof && m_outcome == ParseOutcome::Continue) { parse_current(); }
	check_required_parsed();
	auto ret = Result{.outcome = m_outcome};
	if (m_frame.command) { ret.command_identifier = m_frame.command->identifier; }
	return ret;
}

void Parser::advance() { m_scanner.scan_next(m_current); }

void Parser::parse_current() {
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
	case Token::Type::Equals: m_printer.printerr_prefixed("unexpected token: '{}'", m_current.lexeme); throw error::Parse{error::Parse::UnexpectedToken};

	default: m_printer.printerr_prefixed("unrecognized token: '{}'", m_current.lexeme); throw error::Internal{error::Internal::UnrecognizedToken};
	}
}

void Parser::parse_argument() {
	if (select_command()) { return; }

	auto const* positional = next_positional();
	if (!positional) {
		if (!m_frame.list_parameter) {
			m_printer.printerr_prefixed("unknown argument: '{}'", m_current.lexeme);
			throw error::Parse{error::Parse::UnknownArgument};
		}
		if (!m_frame.list_parameter->parse(m_current.lexeme)) {
			m_printer.printerr_prefixed("invalid {}: '{}'", m_frame.list_parameter->name, m_current.lexeme);
			throw error::Parse{error::Parse::InvalidArgument};
		}
		advance();
		return;
	}

	if (!positional->parse(m_current.lexeme)) {
		m_printer.printerr_prefixed("invalid {}: '{}'", positional->name, m_current.lexeme);
		throw error::Parse{error::Parse::InvalidArgument};
	}
	advance();
}

void Parser::parse_long_option() {
	assert(m_current.lexeme.starts_with("--") && m_current.lexeme.size() > 2);

	auto const option_token = m_current;
	auto const word = option_token.lexeme.substr(2);

	if (!m_frame.command && !m_program.version.empty() && word == "version") {
		m_printer.printer->println(m_program.version);
		m_outcome = ParseOutcome::EarlyExit;
		return;
	}

	if (word == "help") {
		// TODO: print global/command help
		m_outcome = ParseOutcome::EarlyExit;
		return;
	}

	auto const& named = get_named(word);
	advance();
	parse_last_option(named, option_token.lexeme);
}

void Parser::parse_short_options() {
	assert(m_current.lexeme.front() == '-' && m_current.lexeme.size() > 1);

	auto letters = m_current.lexeme.substr(1);
	for (; letters.size() > 1; letters.remove_prefix(1)) {
		auto const& named = get_named(letters.front());
		if (!named.is_flag) {
			m_printer.printerr_prefixed("option requires an argument -- '{}'", named.letter);
			throw error::Parse{error::Parse::OptionRequiresArgument};
		}
		named.parse("true");
	}

	auto const& named = get_named(letters.front());
	advance();
	parse_last_option(named, letters);
}

void Parser::parse_last_option(parameter::Named const& named, std::string_view const option_lexeme) {
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

void Parser::parse_option_value(parameter::Named const& named, std::string_view const option_lexeme) {
	if (m_current.type != Token::Type::String) {
		m_printer.printerr_prefixed("option '{}' requires an argument", option_lexeme);
		throw error::Parse{error::Parse::OptionRequiresArgument};
	}

	if (!named.parse(m_current.lexeme)) {
		m_printer.printerr_prefixed("invalid '{}': {}", named.word, m_current.lexeme);
		throw error::Parse{error::Parse::InvalidArgument};
	}

	advance();
}

auto Parser::find_command(std::string_view const identifier) const -> Command const* {
	auto const it = std::ranges::find_if(m_commands, [identifier](Command const& c) { return c.identifier == identifier; });
	if (it == m_commands.end()) { return nullptr; }
	return &*it;
}

template <typename T>
auto Parser::get_named(T const t) const -> parameter::Named const& {
	auto const pred = [t](parameter::Named const* n) {
		if constexpr (std::same_as<T, char>) {
			return n->letter == t;
		} else {
			return n->word == t;
		}
	};
	auto const it = std::ranges::find_if(m_frame.named_parameters, pred);
	if (it != m_frame.named_parameters.end()) { return **it; }
	m_printer.printerr_prefixed("unrecognized option: '{}'", m_current.lexeme);
	throw error::Parse{error::Parse::UnrecognizedOption};
}

auto Parser::select_command() -> bool {
	if (m_frame.command) { return false; }
	auto const* command = find_command(m_current.lexeme);
	if (!command) { return false; }
	m_frame.recreate(m_printer, *command);
	m_positional_index = 0;
	advance();
	return true;
}

auto Parser::next_positional() -> parameter::Positional const* {
	if (m_positional_index >= m_frame.positional_parameters.size()) { return nullptr; }
	return m_frame.positional_parameters[m_positional_index++];
}

void Parser::check_required_parsed() {
	auto const* remaining = next_positional();
	if (!remaining || remaining->type == parameter::Type::Optional) { return; }
	m_printer.printerr_prefixed("missing required argument: '{}'", remaining->name);
	throw error::Parse{error::Parse::MissingRequiredArgument};
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
