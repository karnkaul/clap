#pragma once
#include "clap/command.hpp"
#include "clap/parameter.hpp"
#include "clap/printer.hpp"
#include "detail/scanner.hpp"
#include <cstddef>
#include <span>
#include <string_view>

namespace clap::detail {
struct ParseProgram {
	std::string_view name{};
	std::string_view version{};
	std::string_view description{};
};

struct ParseInput {
	std::span<std::string_view const> args;
	std::span<Parameter const> parameters{};
	std::span<Command const> commands{};
	ParseProgram program{};
	IPrinter* printer{};
};

enum class ParseOutcome : std::int8_t { Continue, EarlyExit };

namespace error {
enum class Internal : std::int8_t {
	UnrecognizedToken,
};

enum class Parameter : std::int8_t {
	ExtraneousPositional,
};

enum class Parse : std::int8_t {
	UnknownArgument,
	InvalidArgument,
	UnrecognizedOption,
	OptionRequiresArgument,
	MissingRequiredArgument,
	UnexpectedToken,
};
} // namespace error

class Parser {
  public:
	explicit Parser(ParseInput const& input);

	auto parse() noexcept(false) -> ParseOutcome;

  private:
	struct PrinterWrapper {
		void printerr_prefixed(std::string_view message) const;

		IPrinter* printer{};
		std::string_view program_name{};
	};

	void triage_parameters(std::span<Parameter const> input);
	void advance();

	void parse_current();
	void parse_argument();
	void parse_long_option();
	void parse_short_options();
	void parse_last_option(parameter::Named const& named, bool is_letter);
	void parse_option_value(parameter::Named const& named, bool is_letter);

	[[nodiscard]] auto find_command(std::string_view identifier) const -> Command const*;
	[[nodiscard]] auto get_named(char letter) const noexcept(false) -> parameter::Named const&;
	[[nodiscard]] auto get_named(std::string_view word) const noexcept(false) -> parameter::Named const&;

	auto select_command() -> bool;
	void check_required_parsed();

	PrinterWrapper m_printer{};

	std::span<std::string_view const> m_args;
	ParseProgram m_program{};

	std::span<Command const> m_commands{};

	std::vector<parameter::Named const*> m_named_parameters{};
	std::vector<parameter::Positional const*> m_positional_parameters{};
	parameter::List const* m_list_parameter{};

	Scanner m_scanner;

	Command const* m_command{};
	std::size_t m_positional_index{};

	Token m_current{};
	bool m_options_terminated{};
	ParseOutcome m_outcome{ParseOutcome::Continue};
};
} // namespace clap::detail
