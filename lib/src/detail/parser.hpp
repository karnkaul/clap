#pragma once
#include "clap/command.hpp"
#include "clap/parameter.hpp"
#include "clap/printer.hpp"
#include "detail/common.hpp"
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
	std::span<Parameter const> parameters{};
	std::span<Command const> commands{};
	ParseProgram program{};
	IPrinter* printer{};
};

struct ParseContext {
	[[nodiscard]] static auto create(ParseInput const& input) noexcept(false) -> ParseContext;

	PrinterWrapper printer{};

	ParseFrame2 main{};
	std::vector<ParseFrame2> commands{};

	std::string_view version{};
	std::string_view description{};
};

enum class ParseOutcome : std::int8_t { Continue, EarlyExit };

namespace error {
enum class Internal : std::int8_t {
	UnrecognizedToken,
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
	struct Result {
		ParseOutcome outcome{};
		std::string_view command_identifier{};
	};

	explicit Parser(ParseInput const& input, std::span<std::string_view const> args);

	auto parse() noexcept(false) -> Result;

  private:
	void advance();

	void parse_current();
	void parse_argument();
	void parse_long_option();
	void parse_short_options();
	void parse_last_option(parameter::Named const& named, std::string_view option_lexeme);
	void parse_option_value(parameter::Named const& named, std::string_view option_lexeme);

	[[nodiscard]] auto find_command(std::string_view identifier) const -> Command const*;
	template <typename T>
	[[nodiscard]] auto get_named(T t) const noexcept(false) -> parameter::Named const&;

	[[nodiscard]] auto select_command() -> bool;
	[[nodiscard]] auto next_positional() -> parameter::Positional const*;
	void check_required_parsed();

	PrinterWrapper m_printer{};
	ParseProgram m_program{};
	std::span<Command const> m_commands{};

	ParseFrame m_frame{};
	std::size_t m_positional_index{};

	Scanner m_scanner;

	Token m_current{};
	bool m_options_terminated{};
	ParseOutcome m_outcome{ParseOutcome::Continue};
};
} // namespace clap::detail
