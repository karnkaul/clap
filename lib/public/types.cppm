export module clap.types;

export import std;

export namespace clap {
enum class ExitCode : std::int8_t {
	Success = 0,
	Failure = 1,

	InvalidParameter = 11,
	UnknownArgument = 12,
	InvalidArgument = 13,
	UnrecognizedOption = 14,
	OptionRequiresArgument = 15,
	MissingRequiredArgument = 16,
	MissingRequiredCommand = 17,
	UnexpectedToken = 18,

	InternalError = 20,
};

enum class Outcome : std::int8_t { Continue, EarlyExit };

enum class CommandPolicy : std::int8_t { Required, Optional };

template <typename... Ts>
struct Visitor : Ts... {
	using Ts::operator()...;
};

class Result {
  public:
	[[nodiscard]] static constexpr auto create_early_exit(ExitCode const exit_code) -> Result {
		return Result{exit_code, Outcome::EarlyExit, {}};
	}
	[[nodiscard]] static constexpr auto create_continue() -> Result {
		return Result{ExitCode::Success, Outcome::Continue, {}};
	}

	explicit constexpr Result(ExitCode const exit_code,
							  Outcome const outcome,
							  std::string_view const command_identifier)
		: m_exit_code(exit_code),
		  m_outcome(outcome),
		  m_command_identifier(command_identifier) {
		if (is_error()) {
			m_outcome = Outcome::EarlyExit;
		}
	}

	[[nodiscard]] constexpr auto exit_code() const -> ExitCode { return m_exit_code; }
	[[nodiscard]] constexpr auto outcome() const -> Outcome { return m_outcome; }
	[[nodiscard]] constexpr auto command_identifier() const -> std::string_view { return m_command_identifier; }

	[[nodiscard]] constexpr auto is_success() const -> bool { return exit_code() == ExitCode::Success; }
	[[nodiscard]] constexpr auto is_error() const -> bool { return !is_success(); }
	[[nodiscard]] constexpr auto should_early_exit() const -> bool { return outcome() == Outcome::EarlyExit; }
	[[nodiscard]] constexpr auto return_code() const -> int { return static_cast<int>(exit_code()); }

	explicit constexpr operator bool() const { return is_success(); }

  private:
	ExitCode m_exit_code{};
	Outcome m_outcome{};
	std::string_view m_command_identifier{};
};

class InvalidParameterException : public std::runtime_error {
  public:
	using std::runtime_error::runtime_error;
};

class Polymorphic {
  public:
	virtual ~Polymorphic() = default;

	Polymorphic() = default;
	Polymorphic(Polymorphic const&) = default;
	Polymorphic(Polymorphic&&) = default;
	Polymorphic& operator=(Polymorphic const&) = default;
	Polymorphic& operator=(Polymorphic&&) = default;
};

class IPrinter : public Polymorphic {
  public:
	virtual void println(std::string_view message) = 0;
	virtual void printerr(std::string_view message) = 0;
};

class DefaultPrinter : public IPrinter {
  public:
	void println(std::string_view const message) override { std::println("{}", message); }
	void printerr(std::string_view const message) override { std::println(std::cerr, "{}", message); }
};

struct Program {
	std::string_view name{};
	std::string_view version{};
	std::string_view description{};
	IPrinter* custom_printer{};
};

/// \returns Filename stem, assuming argv_0 is a path to the executable.
[[nodiscard]] auto to_program_name(std::string_view argv_0) -> std::string;
/// \returns Whitespace-stripped list of words.
/// Supports quoted strings.
[[nodiscard]] auto to_words(std::string_view line) -> std::vector<std::string_view>;

[[nodiscard]] auto or_default(IPrinter* custom_printer) -> IPrinter&;

[[nodiscard]] constexpr auto strip_quotes(std::string_view in) {
	if (in.starts_with('"')) {
		in.remove_prefix(1);
		if (in.ends_with('"')) {
			in.remove_suffix(1);
		}
	}
	return in;
}
} // namespace clap
