#pragma once
#include "clap/command.hpp"
#include "clap/parameter.hpp"
#include "clap/printer.hpp"
#include "clap/program.hpp"
#include "clap/result.hpp"
#include <memory>
#include <span>
#include <string_view>

namespace clap {
namespace detail {
struct Context;
} // namespace detail

/// \returns Filename stem, assuming argv_0 is a path to the executable.
[[nodiscard]] auto to_program_name(std::string_view argv_0) -> std::string;
/// \returns Whitespace-stripped list of words.
/// Supports quoted strings.
[[nodiscard]] auto to_words(std::string_view line) -> std::vector<std::string_view>;

enum class CommandPolicy : std::int8_t { Required, Optional };

class Parser {
  public:
	/// \brief Create a Parser for a list of Parameters.
	/// Each parameter can be named or positional, the last positional may be optional or a list.
	explicit Parser(ParameterList parameters, Program const& program = {}, IPrinter* custom_printer = {}) noexcept(false);

	/// \brief Create a Parser for a list of Commands.
	/// The base/main frame can pass bound options (parameter::Named) but not arguments (parameter::Positional).
	explicit Parser(CommandList commands, OptionList options = {}, Program const& program = {}, IPrinter* custom_printer = {}) noexcept(false);

	/// \returns Result of parsing passed words.
	[[nodiscard]] auto parse_words(std::span<std::string_view const> words) const -> Result;
	/// \returns Result of parsing passed line split into words.
	[[nodiscard]] auto parse_line(std::string_view line) const -> Result;
	/// \returns Result of parsing args to main as words.
	[[nodiscard]] auto parse_main(int argc, char const* const* argv, bool skip_argv_0 = true) const -> Result;

	/// \returns Help text (that gets printed on "--help").
	[[nodiscard]] auto get_help_text() const -> std::string;

	/// \brief Whether to treat a missing command as an error.
	/// Only relevant for Command Parsers.
	CommandPolicy command_policy{CommandPolicy::Required};

  private:
	struct Deleter {
		void operator()(detail::Context* ptr) const noexcept;
	};

	ParameterList m_parameters{};
	OptionList m_options{};
	CommandList m_commands{};

	std::unique_ptr<detail::Context, Deleter> m_context{};
};
} // namespace clap
