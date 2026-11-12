export module clap;

export import clap.build_version;

#pragma region types

export namespace clap {
enum class ExitCode : std::int8_t {
	Success = 0,
	Failure = 1,

	InvalidParser = 101,
	UnknownArgument = 102,
	InvalidArgument = 103,
	UnrecognizedOption = 104,
	OptionRequiresArgument = 105,
	MissingRequiredArgument = 106,
	MissingRequiredCommand = 107,
	UnexpectedToken = 108,
	InternalError = 110,
};

enum class Outcome : std::int8_t { Continue, EarlyExit };

enum class CommandPolicy : std::int8_t { Required, Optional };

class Result {
  public:
	[[nodiscard]] static constexpr auto error(ExitCode exit_code) -> Result {
		return Result{exit_code, Outcome::EarlyExit, {}};
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

struct Program {
	std::string_view name{};
	std::string_view version{};
	std::string_view description{};
};

class InvalidParameterException : public std::runtime_error {
  public:
	using std::runtime_error::runtime_error;
};
} // namespace clap

#pragma endregion

#pragma region Parameter

export namespace clap {
namespace parameter {
template <typename T>
concept FlagT = std::same_as<T, bool>;

template <typename T>
concept NumberT = !FlagT<T> && (std::integral<T> || std::floating_point<T>);

template <typename T>
concept StringT = !FlagT<T> && (std::same_as<T, std::string> || std::same_as<T, std::string_view>);

template <typename T>
concept ValueT = NumberT<T> || StringT<T>;

enum class Type : std::int8_t { Required, Optional };

void split_named_key(std::string_view key, char& out_letter, std::string_view& out_word);

[[nodiscard]] auto parse_to(bool& out, std::string_view input) -> bool;

template <NumberT T>
[[nodiscard]] auto parse_to(T& out, std::string_view const input) -> bool {
	auto const* end = input.data() + input.size();
	auto [ptr, ec] = std::from_chars(input.data(), end, out);
	return ec == std::errc{} && ptr == end;
}

template <StringT T>
[[nodiscard]] auto parse_to(T& out, std::string_view const input) -> bool {
	if (input.empty()) {
		return false;
	}
	out = input;
	return true;
}

template <ValueT T>
[[nodiscard]] auto parse_to(std::vector<T>& out, std::string_view const input) -> bool {
	auto t = T{};
	if (!parse_to(t, input)) {
		return false;
	}
	out.push_back(std::move(t));
	return true;
}

struct Parse {
	auto operator()(std::string_view input) const -> bool;

	std::function<bool(std::string_view)> func{};
	bool* was_set{};
};

template <typename T>
[[nodiscard]] auto create_parse(T& out, bool* was_set) -> Parse {
	if (was_set) {
		*was_set = false;
	}
	return Parse{
		.func =
			[&out](std::string_view const input) {
				return parse_to(out, input);
			},
		.was_set = was_set,
	};
}

/// \brief Named parameter (option / flag).
struct Named {
	template <typename T>
		requires(ValueT<T> || FlagT<T>)
	explicit Named(T& out, std::string_view const key, std::string_view const description, bool* was_set)
		: description(description),
		  parse(create_parse(out, was_set)),
		  is_flag(FlagT<T>) {
		split_named_key(key, letter, word);
	}

	char letter{};
	std::string_view word{};
	std::string_view description;
	Parse parse;
	bool is_flag;
};

/// \brief Postional parameter (argument).
struct Positional {
	template <ValueT T>
	explicit Positional(
		T& out, std::string_view const name, std::string_view const description, Type const type, bool* was_set)
		: name(name),
		  description(description),
		  type(type),
		  parse(create_parse(out, was_set)) {}

	std::string_view name;
	std::string_view description;
	Type type;
	Parse parse;
};

/// \brief Positional list.
struct List {
	template <ValueT T>
	explicit List(std::vector<T>& out, std::string_view const name, std::string_view const description, bool* was_set)
		: name(name),
		  description(description),
		  parse(create_parse(out, was_set)) {}

	std::string_view name;
	std::string_view description;
	Parse parse;
};
} // namespace parameter

using Parameter = std::variant<parameter::Named, parameter::Positional, parameter::List>;
using ParameterList = std::vector<Parameter>;
using OptionList = std::vector<parameter::Named>;

/// \returns parameter::Named with bound flag.
template <parameter::FlagT T>
[[nodiscard]] auto
named_flag(T& out, std::string_view const key, std::string_view const description = {}, bool* was_set = {}) {
	return parameter::Named{out, key, description, was_set};
}

/// \returns parameter::Named with bound value.
template <parameter::ValueT T>
[[nodiscard]] auto
named_option(T& out, std::string_view const key, std::string_view const description = {}, bool* was_set = {}) {
	return parameter::Named{out, key, description, was_set};
}

/// \returns Required parameter::Positional with bound value.
template <parameter::ValueT T>
[[nodiscard]] auto
positional_required(T& out, std::string_view const name, std::string_view const description = {}, bool* was_set = {}) {
	return parameter::Positional{out, name, description, parameter::Type::Required, was_set};
}

/// \returns Optional parameter::Positional with bound value.
/// Only one optional or list parameter can be present, and as the last positional parameter.
template <parameter::ValueT T>
[[nodiscard]] auto
positional_optional(T& out, std::string_view const name, std::string_view const description = {}, bool* was_set = {}) {
	return parameter::Positional{out, name, description, parameter::Type::Optional, was_set};
}

/// \returns parameter::List with bound vector of values.
/// Only one list or optional parameter can be present, and as the last positional parameter.
template <parameter::ValueT T>
[[nodiscard]] auto positional_list(std::vector<T>& out,
								   std::string_view const name,
								   std::string_view const description = {},
								   bool* was_set = {}) {
	return parameter::List{out, name, description, was_set};
}
} // namespace clap

#pragma endregion

#pragma region IPrinter

export namespace clap {
class IPrinter {
  public:
	virtual ~IPrinter() = default;

	IPrinter() = default;
	IPrinter(IPrinter const&) = default;
	IPrinter(IPrinter&&) = default;
	IPrinter& operator=(IPrinter const&) = default;
	IPrinter& operator=(IPrinter&&) = default;

	[[nodiscard]] static auto default_printer() -> IPrinter&;

	virtual void println(std::string_view message) = 0;
	virtual void printerr(std::string_view message) = 0;
};
} // namespace clap

#pragma endregion

#pragma region Command

export namespace clap {
/// \brief Command with its own ParameterList.
struct Command {
	explicit Command(std::string_view identifier,
					 ParameterList parameters,
					 std::string_view description,
					 std::string_view epilogue)
		: identifier(identifier),
		  description(description),
		  parameters(std::move(parameters)),
		  epilogue(epilogue) {}

	std::string_view identifier;
	std::string_view description;
	ParameterList parameters;
	std::string_view epilogue;
};
using CommandList = std::vector<Command>;

/// \returns Command.
[[nodiscard]] auto command(std::string_view identifier,
						   ParameterList parameters,
						   std::string_view description = {},
						   std::string_view epilogue = {}) -> Command;
} // namespace clap

#pragma endregion

#pragma region spec

export namespace clap::spec {
struct Parameters {
	/// \brief Each parameter can be named or positional, the last positional may be optional or a list.
	ParameterList parameters{};

	Program program{};
	IPrinter* custom_printer{};
};

struct Commands {
	/// \brief Only options (named parameters) are supported with commands.
	OptionList options{};
	/// \brief One or more Commands each with their own ParameterLists.
	CommandList commands{};
	/// \brief Whether a command selection is required.
	CommandPolicy policy{CommandPolicy::Required};

	Program program{};
	IPrinter* custom_printer{};
};
} // namespace clap::spec

#pragma endregion

#pragma region Parser

export namespace clap {
namespace detail {
class IContext {
  public:
	virtual ~IContext() = default;
};
} // namespace detail

/// \returns Filename stem, assuming argv_0 is a path to the executable.
[[nodiscard]] auto to_program_name(std::string_view argv_0) -> std::string;
/// \returns Whitespace-stripped list of words.
/// Supports quoted strings.
[[nodiscard]] auto to_words(std::string_view line) -> std::vector<std::string_view>;

class Parser {
  public:
	/// \brief Create a Parser for a list of Parameters.
	explicit Parser(spec::Parameters spec) noexcept(false);
	/// \brief Create a Parser for a list of Commands.
	explicit Parser(spec::Commands spec) noexcept(false);

	/// \returns Result of parsing passed words.
	[[nodiscard]] auto parse_words(std::span<std::string_view const> words) const -> Result;
	/// \returns Result of parsing passed line split into words.
	[[nodiscard]] auto parse_line(std::string_view line) const -> Result;
	/// \returns Result of parsing args to main as words.
	[[nodiscard]] auto parse_main(int argc, char const* const* argv, bool skip_argv_0 = true) const -> Result;

	/// \returns Help text (that gets printed on "--help").
	[[nodiscard]] auto get_help_text() const -> std::string;

  private:
	std::unique_ptr<detail::IContext> m_context{};
};
} // namespace clap

#pragma endregion
