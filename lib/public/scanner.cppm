export module clap.scanner;

export import std;

export namespace clap {
struct ScannerToken {
	enum class Type : std::int8_t { Eof, String, MinusString, MinusMinusString, MinusMinus };

	Type type{};
	std::string_view lexeme{};
};

class Scanner {
  public:
	using Token = ScannerToken;

	explicit Scanner(std::span<std::string_view const> words);

	auto scan_next(Token& out) -> bool;

	[[nodiscard]] auto get_remain() const -> std::span<std::string_view const> { return m_remain; }

  private:
	auto scan_next() -> Token;
	auto scan_quoted() -> Token;

	void advance();

	[[nodiscard]] auto to_token(Token::Type type, std::size_t length = 0uz) -> Token;

	std::span<std::string_view const> m_remain{};
	std::span<std::string_view const> m_unscanned{};
	std::string_view m_current{};
	std::size_t m_scanned{};
};
} // namespace clap
