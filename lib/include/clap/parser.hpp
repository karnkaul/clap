#pragma once
#include "clap/result.hpp"
#include "clap/spec.hpp"
#include <memory>
#include <span>

namespace clap {
namespace detail {
class Context;
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

	/// \brief Whether to treat a missing command as an error.
	/// Only relevant for Command Parsers.
	CommandPolicy command_policy{CommandPolicy::Required};

  private:
	struct Deleter {
		void operator()(detail::Context* ptr) const noexcept;
	};
	std::unique_ptr<detail::Context, Deleter> m_context{};
};
} // namespace clap
