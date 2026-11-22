export module clap.parser;

export import clap.spec;
export import clap.scanner;

export namespace clap {
class IContext : public Polymorphic {
  public:
	[[nodiscard]] virtual auto get_named_parameters() const -> std::span<parameter::Named const> = 0;

	virtual void print_help_text() = 0;
	[[nodiscard]] virtual auto has_version() const -> bool = 0;
	virtual void print_version() = 0;

	virtual void print_error(std::string_view message) = 0;
};

class OptionParser {
  public:
	using Token = ScannerToken;

	enum class Iterate : std::int8_t { Consume, Pass, EarlyExit };

	using Action = std::variant<Iterate, Result>;

	explicit OptionParser(IContext& context, std::span<parameter::Named const> options);

	[[nodiscard]] auto parse_next(Token const& token) -> Action;
	[[nodiscard]] auto finish() -> Result;

	std::vector<Token> positional_tokens{};

  private:
	[[nodiscard]] auto parse_long(Token const& token) -> Action;
	[[nodiscard]] auto parse_shorts(Token const& token) -> Action;

	[[nodiscard]] auto
	parse_option_value(parameter::Named const& named, std::string_view value, std::string_view option_lexeme) -> Result;

	[[nodiscard]] auto assign_value_to(parameter::Named const& named,
									   std::string_view value,
									   std::string_view option_lexeme) const -> Result;

	template <typename T>
	[[nodiscard]] auto find_named(T t) const -> parameter::Named const*;

	IContext& m_context;

	std::span<parameter::Named const> m_options{};

	bool m_force_ended{};
	parameter::Named const* m_expects_value{};
	std::string_view m_option_lexeme{};
};
} // namespace clap
