#include "clap/parameter.hpp"
#include "detail/parser.hpp"
#include "detail/scanner.hpp"
#include "detail/visitor.hpp"
#include <algorithm>
#include <cassert>
#include <limits>
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

Parser::Parser(ParseInput const& input) : m_args(input.args), m_program(input.program), m_commands(input.commands), m_scanner(input.args) {
	triage_parameters(input.parameters);
}

auto Parser::parse() -> ParseOutcome {
	advance();
	while (m_current.type != Token::Type::Eof && m_outcome == ParseOutcome::Continue) { parse_current(); }
	check_required_parsed();
	return m_outcome;
}

void Parser::triage_parameters(std::span<Parameter const> input) {
	m_named_parameters.clear();
	m_positional_parameters.clear();
	m_list_parameter = nullptr;
	m_positional_index = 0;

	auto positionals_ended = false;
	auto const visitor = Visitor{
		[&](parameter::Named const& n) { m_named_parameters.push_back(&n); },
		[&](parameter::Positional const& p) {
			if (positionals_ended) {
				// TODO: print error
				throw ParameterError{ParameterError::ExtraneousPositional};
			}
			if (p.type == parameter::Type::Optional) { positionals_ended = true; }
			m_positional_parameters.push_back(&p);
		},
		[&](parameter::List const& l) {
			if (positionals_ended) {
				// TODO: print error
				throw ParameterError{ParameterError::ExtraneousPositional};
			}
			m_list_parameter = &l;
			positionals_ended = true;
		},
	};

	for (auto const& parameter : input) { std::visit(visitor, parameter); }
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

	case Token::Type::Equals:
		// TODO: print error
		throw ParseError{ParseError::UnexpectedToken};

	default:
	case Token::Type::Eof:
		// TODO: print error
		throw ParseError{ParseError::UnexpectedToken};
	}
}

void Parser::parse_argument() {
	if (select_command()) { return; }

	if (m_positional_index >= m_positional_parameters.size()) {
		if (!m_list_parameter) {
			// TODO: print error
			throw ParseError{ParseError::ExtraneousArgument};
		}
		if (!m_list_parameter->parse(m_current.lexeme)) {
			// TODO: print error
			throw ParseError{ParseError::InvalidArgument};
		}
		advance();
		return;
	}

	auto const* positional = m_positional_parameters[m_positional_index++];
	if (!positional->parse(m_current.lexeme)) {
		// TODO: print error
		throw ParseError{ParseError::InvalidArgument};
	}
	advance();
}

void Parser::parse_long_option() {
	assert(m_current.lexeme.starts_with("--"));
	auto const word = m_current.lexeme.substr(2);

	if (!m_command && !m_program.version.empty() && word == "version") {
		// TODO: print version
		m_outcome = ParseOutcome::EarlyExit;
		return;
	}

	if (word == "help") {
		// TODO: print global/command help
		m_outcome = ParseOutcome::EarlyExit;
		return;
	}

	if (word == "usage") {
		// TODO: print global/command usage
		m_outcome = ParseOutcome::EarlyExit;
		return;
	}

	auto const* named = find_named(word);
	if (!named) {
		// TODO: print error
		throw ParseError{ParseError::InvalidArgument};
	}

	advance();
	parse_last_option(*named);
}

void Parser::parse_short_options() {
	assert(m_current.lexeme.front() == '-');
	auto letters = m_current.lexeme.substr(1);
	for (; letters.size() > 1; letters.remove_prefix(1)) {
		auto const* named = find_named(letters.front());
		if (!named || !named->is_flag) {
			// TODO: print error
			throw ParseError{ParseError::OptionRequiresArgument};
		}
		named->parse("true");
	}

	auto const* named = find_named(letters.front());
	if (!named) {
		// TODO: print error
		throw ParseError{ParseError::InvalidArgument};
	}

	advance();
	parse_last_option(*named);
}

void Parser::parse_last_option(parameter::Named const& named) {
	if (m_current.type == Token::Type::Equals) {
		advance();
		parse_option_value(named);
		return;
	}

	if (named.is_flag) {
		named.parse("true");
		return;
	}

	if (m_current.type != Token::Type::String) {
		// TODO: print error
		throw ParseError{ParseError::MissingRequiredArgument};
	}

	parse_option_value(named);
}

void Parser::parse_option_value(parameter::Named const& named) {
	if (m_current.type != Token::Type::String) {
		// TODO: print error
		throw ParseError{ParseError::InvalidArgument};
	}

	if (!named.parse(m_current.lexeme)) {
		// TODO: print error
		throw ParseError{ParseError::InvalidArgument};
	}

	advance();
}

auto Parser::find_named(std::string_view const word) const -> parameter::Named const* {
	auto const it = std::ranges::find_if(m_named_parameters, [word](parameter::Named const* p) { return p->word == word; });
	if (it == m_named_parameters.end()) { return nullptr; }
	return *it;
}

auto Parser::find_named(char const letter) const -> parameter::Named const* {
	auto const it = std::ranges::find_if(m_named_parameters, [letter](parameter::Named const* p) { return p->letter == letter; });
	if (it == m_named_parameters.end()) { return nullptr; }
	return *it;
}

auto Parser::find_command(std::string_view const identifier) const -> Command const* {
	auto const it = std::ranges::find_if(m_commands, [identifier](Command const& c) { return c.identifier == identifier; });
	if (it == m_commands.end()) { return nullptr; }
	return &*it;
}

auto Parser::select_command() -> bool {
	if (m_command) { return false; }
	auto const* command = find_command(m_current.lexeme);
	if (!command) { return false; }
	m_command = command;
	triage_parameters(m_command->parameters);
	advance();
	return true;
}

void Parser::check_required_parsed() {
	if (m_positional_index >= m_positional_parameters.size()) { return; }
	auto const* remaining = m_positional_parameters[m_positional_index];
	if (remaining->type == parameter::Type::Optional) { return; }
	// TODO: print error
	throw ParseError{ParseError::MissingRequiredArgument};
}
} // namespace detail

auto detail::to_string_view(ParseError const error) -> std::string_view {
	switch (error) {
	case ParseError::ExtraneousArgument: return "ExtraneousArgument";
	case ParseError::InvalidArgument: return "InvalidArgument";
	case ParseError::OptionRequiresArgument: return "OptionRequiresArgument";
	case ParseError::MissingRequiredArgument: return "MissingRequiredArgument";
	case ParseError::UnexpectedToken: return "UnexpectedToken";
	}
}

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
