#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace clap::detail {
struct Token {
	enum class Type : std::int8_t { Eof, String, MinusString, MinusMinusString, Equals, MinusMinus };

	Type type{};
	std::string_view lexeme{};
};

class Scanner {
  public:
	explicit Scanner(std::span<std::string_view const> words);

	auto scan_next(Token& out) -> bool;

  private:
	auto scan_next() -> Token;
	auto minus(Token::Type type) -> Token;
	auto scan_string() -> Token;

	void advance();

	auto to_token(Token::Type type, std::size_t length) -> Token;

	std::span<std::string_view const> m_remain{};
	std::string_view m_current{};
};
} // namespace clap::detail
