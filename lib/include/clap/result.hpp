#pragma once
#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace clap {
enum class ExitCode : std::int8_t {
	Success = EXIT_SUCCESS,
	Failure = EXIT_FAILURE,

	InvalidParser = 101,
	UnknownArgument = 102,
	InvalidArgument = 103,
	UnrecognizedOption = 104,
	OptionRequiresArgument = 105,
	MissingRequiredArgument = 106,
	UnexpectedToken = 107,
	InternalError = 110,
};

enum class Outcome : std::int8_t { Continue, EarlyExit };

class Result {
  public:
	[[nodiscard]] static constexpr auto error(ExitCode exit_code) -> Result { return Result{exit_code, Outcome::EarlyExit, {}}; }

	explicit constexpr Result(ExitCode const exit_code, Outcome const outcome, std::string_view const command_identifier)
		: m_exit_code(exit_code), m_outcome(outcome), m_command_identifier(command_identifier) {
		if (is_error()) { m_outcome = Outcome::EarlyExit; }
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
} // namespace clap
