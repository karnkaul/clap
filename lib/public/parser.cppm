export module clap.parser;

export import clap.spec;
export import clap.scanner;

export namespace clap {
class IContext : public Polymorphic {
  public:
	virtual void print_help_text() = 0;
	[[nodiscard]] virtual auto has_version() const -> bool = 0;
	virtual void print_version() = 0;

	virtual void print_error(std::string_view message) = 0;
};

class OptionParser {
  public:
	using Token = ScannerToken;

	enum class Iterate : std::int8_t { Consume, Pass };

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

class ParameterParser {
  public:
	explicit ParameterParser(IContext& context) : m_context(context) {}

	auto set_parameters(ParameterList parameters) -> Result;

	auto parse(Scanner& scanner) -> Result;

  private:
	void clear_parameters();

	void add_option(parameter::Named option);
	auto add_positional(parameter::Positional positional) -> Result;
	auto add_list(parameter::List list) -> Result;

	auto parse_arguments() -> Result;
	auto parse_front_to_list() -> Result;
	auto parse_back_to_list() -> Result;
	auto parse_list() -> Result;

	auto check_required(parameter::Positional const& positional) -> Result;
	auto parse_positional(parameter::Positional const& positional, std::string_view value) -> Result;

	IContext& m_context;

	std::vector<parameter::Named> m_options{};
	std::vector<parameter::Positional> m_pre_list{};
	std::optional<parameter::List> m_list{};
	std::vector<parameter::Positional> m_post_list{};
	bool m_optional_positionals{};

	std::vector<ScannerToken> m_argument_tokens{};
	std::span<ScannerToken const> m_arguments{};
};

class CommandParser {
  public:
	explicit CommandParser(IContext& context);

	auto set_parameters(OptionList program_options, CommandList commands, CommandPolicy policy) -> Result;

	auto parse(Scanner& scanner) -> Result;

  private:
	using Token = ScannerToken;

	void clear_parameters();

	auto parse_program_options(Scanner& scanner) -> Result;
	auto select_command() -> Result;
	auto parse_command_parameters(Scanner& scanner) -> Result;

	struct Entry {
		Command command;
		ParameterParser parameter_parser;
	};

	IContext& m_context;

	std::vector<Entry> m_entries{};
	OptionList m_program_options{};
	CommandPolicy m_policy{};

	Token m_command_token{};
	Entry* m_selected_command{};
};
} // namespace clap
