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

[[nodiscard]] auto to_program_name(std::string_view argv_0) -> std::string;
[[nodiscard]] auto to_words(std::string_view line) -> std::vector<std::string_view>;

class Parser {
  public:
	explicit Parser(ParameterList parameters, CommandList commands, Program const& program = {}, IPrinter* custom_printer = {});
	explicit Parser(ParameterList parameters, Program const& program = {}, IPrinter* custom_printer = {});

	[[nodiscard]] auto parse_words(std::span<std::string_view const> words) const -> Result;
	[[nodiscard]] auto parse_line(std::string_view line) const -> Result;
	[[nodiscard]] auto parse_main(int argc, char const* const* argv, bool skip_argv_0 = true) const -> Result;

  private:
	struct Deleter {
		void operator()(detail::Context* ptr) const noexcept;
	};

	ParameterList m_parameters;
	CommandList m_commands;

	std::unique_ptr<detail::Context, Deleter> m_context{};
};
} // namespace clap
