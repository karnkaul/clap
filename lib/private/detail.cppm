export module clap.detail;

export import clap;

export namespace clap::detail {
enum class Error : std::int8_t {
	// internal
	UnrecognizedToken,

	// parse
	UnknownArgument,
	InvalidArgument,
	UnrecognizedOption,
	OptionRequiresArgument,
	MissingRequiredArgument,
	MissingRequiredCommand,
	UnexpectedToken,
};

[[nodiscard]] constexpr auto to_exit_code(Error const err) {
	switch (err) {
	case Error::UnrecognizedToken: return ExitCode::InternalError;
	case Error::UnknownArgument: return ExitCode::UnknownArgument;
	case Error::InvalidArgument: return ExitCode::InvalidArgument;
	case Error::UnrecognizedOption: return ExitCode::UnrecognizedOption;
	case Error::OptionRequiresArgument: return ExitCode::OptionRequiresArgument;
	case Error::MissingRequiredArgument: return ExitCode::MissingRequiredArgument;
	case Error::MissingRequiredCommand: return ExitCode::MissingRequiredCommand;
	case Error::UnexpectedToken: return ExitCode::UnexpectedToken;
	default: return ExitCode::Failure;
	}
}

template <typename T>
using Ptr = T*;

template <typename... Ts>
struct Visitor : Ts... {
	using Ts::operator()...;
};

struct PrinterWrapper {
	explicit PrinterWrapper(Ptr<IPrinter> printer, std::string_view program_name) noexcept(true);

	template <typename... Args>
	void prefixed_err(std::format_string<Args...> fmt, Args&&... args) const {
		auto message = std::string{program_name};
		if (!message.empty()) {
			message += ": ";
		}
		std::format_to(std::back_inserter(message), fmt, std::forward<Args>(args)...);
		printer->printerr(message);
	}

	auto operator->() const { return printer; }

	Ptr<IPrinter> printer{};
	std::string_view program_name{};
};

struct Frame {
	explicit Frame(std::span<Parameter const> parameters) noexcept(false);
	explicit Frame(std::span<parameter::Named const> options) noexcept(true);

	Ptr<Command const> command{};
	std::vector<Ptr<parameter::Named const>> named_parameters{};
	std::vector<Ptr<parameter::Positional const>> positional_parameters{};
	Ptr<parameter::List const> list_parameter{};
};

class Context : public IContext {
	ParameterList m_parameters{};
	OptionList m_options{};
	CommandList m_commands{};

  public:
	Context(Context const&) = delete;
	Context(Context&&) = delete;
	Context& operator=(Context&&) = delete;
	Context& operator=(Context const&) = delete;

	~Context() = default;

	explicit Context(spec::Parameters spec) noexcept(false);
	explicit Context(spec::Commands spec) noexcept(false);

	void override_program_name_if_empty(std::string_view program_name);

	PrinterWrapper printer;

	Frame main;
	std::vector<Frame> commands{};

	std::string_view version{};
	std::string_view description{};
	CommandPolicy command_policy{};
};

struct Token {
	enum class Type : std::int8_t { Eof, String, MinusString, MinusMinusString, MinusMinus };

	Type type{};
	std::string_view lexeme{};
};

class Scanner {
  public:
	explicit Scanner(std::span<std::string_view const> words);

	auto scan_next(Token& out) -> bool;

  private:
	auto scan_next() -> Token;
	auto scan_quoted() -> Token;

	void advance();

	auto to_token(Token::Type type, std::size_t length = 0uz) -> Token;

	std::span<std::string_view const> m_remain{};
	std::string_view m_current{};
};

class ParserImpl {
  public:
	struct Result {
		Outcome outcome{};
		std::string_view command_identifier{};
	};

	explicit ParserImpl(Context const& context, std::span<std::string_view const> args) noexcept(true);

	auto parse() noexcept(false) -> Result;

	[[nodiscard]] auto help_text() const -> std::string { return m_environment.help_text(); }

  private:
	struct Environment {
		explicit Environment(Context const& context) noexcept(false);

		[[nodiscard]] auto next_positional() -> Ptr<parameter::Positional const>;
		[[nodiscard]] auto set_command_frame(std::string_view identifier) -> bool;

		[[nodiscard]] auto help_text_parameters(bool for_command) const -> std::string;
		[[nodiscard]] auto help_text_commands() const -> std::string;

		[[nodiscard]] auto help_text() const -> std::string;

		[[nodiscard]] auto is_missing_required_command() const -> bool;

		PrinterWrapper printer;

		std::span<Frame const> commands{};
		std::string_view version{};
		std::string_view description{};
		CommandPolicy command_policy{};

		Ptr<Frame const> frame{};
		std::size_t positional_index{};
	};

	void advance();

	void parse_current();
	void parse_argument();
	void parse_long_option();
	void parse_short_options();
	void parse_option_value(parameter::Named const& named, std::string_view value, std::string_view option_lexeme);

	template <typename T>
	[[nodiscard]] auto get_named(T t) const noexcept(false) -> parameter::Named const&;

	[[nodiscard]] auto select_command() -> bool;
	void check_required_parsed();

	Environment m_environment;
	Scanner m_scanner;

	Token m_current{};
	bool m_options_terminated{};
	Outcome m_outcome{Outcome::Continue};
};
} // namespace clap::detail
