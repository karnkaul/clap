module clap.scanner;

import clap.types;

namespace clap {
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
	if (m_current.starts_with('"')) {
		return scan_quoted();
	}

	if (m_current == "--") {
		return to_token(Token::Type::MinusMinus, 2);
	}

	if (m_current.starts_with("--")) {
		return to_token(Token::Type::MinusMinusString);
	}
	if (m_current.starts_with('-')) {
		return to_token(Token::Type::MinusString);
	}

	return to_token(Token::Type::String);
}

auto Scanner::scan_quoted() -> Token {
	m_current = strip_quotes(m_current);
	return to_token(Token::Type::String);
}

void Scanner::advance() {
	if (m_remain.empty()) {
		m_current = {};
		return;
	}
	m_current = m_remain.front();
	m_remain = m_remain.subspan(1);
}

auto Scanner::to_token(Token::Type const type, std::size_t length) -> Token {
	if (length == 0) {
		length = m_current.size();
	}
	auto const ret = Token{
		.type = type,
		.lexeme = m_current.substr(0, length),
	};
	m_current.remove_prefix(length);
	if (m_current.empty()) {
		advance();
	}
	return ret;
}
} // namespace clap
