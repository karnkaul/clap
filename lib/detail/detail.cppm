export module clap.detail;

export import clap.spec;
export import clap.scanner;

export namespace clap::detail {
class IContext : public Polymorphic {
  public:
	virtual void print_help_text() = 0;
	[[nodiscard]] virtual auto has_version() const -> bool = 0;
	virtual void print_version() = 0;

	virtual void print_error(std::string_view message) = 0;
};

class ParseException : public std::runtime_error {
  public:
	explicit ParseException(std::string const& message, ExitCode exit_code);

	[[nodiscard]] auto get_exit_code() const -> ExitCode { return m_exit_code; }

  private:
	ExitCode m_exit_code;
};

class ErrorPrinter {
  public:
	void print_error(std::string_view message) const;

	IPrinter* printer{};
	std::string_view program_name{};
	std::string_view command_identifier{};
};

class ParameterEnvironment {
  public:
	struct Arguments {
		std::vector<parameter::Positional> pre_list{};
		std::optional<parameter::List> list{};
		std::vector<parameter::Positional> post_list{};
	};

	ParameterEnvironment() = default;

	explicit ParameterEnvironment(ErrorPrinter const& printer, ParameterList parameters) noexcept(false);

	void reset_parse_states();

	std::vector<parameter::Named> options;
	Arguments arguments;
};

class CommandEnvironment {
  public:
	explicit CommandEnvironment(ErrorPrinter const& printer, Command command) noexcept(false);

	void reset_parse_states();

	std::string_view identifier;
	std::string_view description;
	ParameterEnvironment parameter_environment;
};

class ProgramEnvironment {
  public:
	explicit ProgramEnvironment(ErrorPrinter const& printer,
								Program const& program,
								std::variant<ParameterList, OptionList> program_parameters,
								CommandList commands,
								CommandPolicy command_policy);

	void reset_parse_states();

	std::string_view program_name;
	std::string_view version;
	std::string_view description;
	ParameterEnvironment parameters;
	std::vector<CommandEnvironment> commands;
	CommandPolicy command_policy;
};

class ProgramContext : public IContext {
  public:
	explicit ProgramContext(Program const& program,
							std::variant<ParameterList, OptionList> program_parameters,
							CommandList commands,
							CommandPolicy command_policy,
							IPrinter* custom_printer);

	void print_help_text() override;
	[[nodiscard]] auto has_version() const -> bool override;
	void print_version() override;

	void print_error(std::string_view message) override { get_error_printer().print_error(message); }

	[[nodiscard]] auto get_error_printer() const -> ErrorPrinter const& { return m_error_printer; }
	[[nodiscard]] auto get_environment() const -> ProgramEnvironment const& { return m_environment; }

	[[nodiscard]] auto help_text_parameters() const -> std::string;
	[[nodiscard]] auto help_text_commands() const -> std::string;
	[[nodiscard]] auto help_text() const -> std::string;

	void set_selected_command(CommandEnvironment const* command_enviroment);
	void reset_environment();

  private:
	ErrorPrinter m_error_printer;
	ProgramEnvironment m_environment;
	IPrinter* m_printer{};

	CommandEnvironment const* m_selected_command{};
};

class OptionParser {
  public:
	using Token = ScannerToken;

	enum class Iterate : std::int8_t { Consume, Pass, EarlyExit };

	explicit OptionParser(IContext& context, std::span<parameter::Named const> options);

	[[nodiscard]] auto parse_next(Token const& token) noexcept(false) -> Iterate;
	void finish() noexcept(false);

  private:
	auto parse_long(Token const& token) -> Iterate;
	void parse_shorts(Token const& token);
	void parse_option_value(parameter::Named const& named, std::string_view value, std::string_view option_lexeme);

	void assign_value_to(parameter::Named const& named, std::string_view value, std::string_view option_lexeme) const;

	template <typename T>
	[[nodiscard]] auto get_named(T t) const -> parameter::Named const&;

	IContext& m_context;

	std::span<parameter::Named const> m_options{};

	bool m_force_ended{};
	parameter::Named const* m_expects_value{};
	std::string_view m_option_lexeme{};
};

class ParameterParser {
  public:
	explicit ParameterParser(IContext& context, ParameterEnvironment const& environment);

	[[nodiscard]] auto parse(Scanner& scanner) noexcept(false) -> Result;

  private:
	using Token = ScannerToken;
	using Iterate = OptionParser::Iterate;

	void parse_arguments();
	void parse_front_to_list();
	void parse_back_to_list();
	void parse_list();

	void check_required(parameter::Positional const& positional);
	void parse_positional(parameter::Positional const& positional, std::string_view value);

	IContext& m_context;
	ParameterEnvironment const& m_environment;

	std::vector<Token> m_argument_tokens{};
	std::span<Token const> m_arguments{};
};

class ProgramParser {
  public:
	explicit ProgramParser(ProgramContext& context);

	[[nodiscard]] auto parse(std::span<std::string_view const> args) -> Result;

  private:
	using Token = ScannerToken;
	using Iterate = OptionParser::Iterate;

	[[nodiscard]] auto parse_parameters(Scanner& scanner) -> Result;
	[[nodiscard]] auto parse_commands(Scanner& scanner) -> Result;

	[[nodiscard]] auto parse_program_options(Scanner& scanner) -> Result;
	void select_command();
	[[nodiscard]] auto parse_command_parameters(Scanner& scanner) -> Result;

	ProgramContext& m_context;

	Token m_command_token{};
	CommandEnvironment const* m_selected_command{};
};
} // namespace clap::detail
