#pragma once
#include "clap/parameter.hpp"
#include "clap/result.hpp"
#include "detail/scanner.hpp"
#include "detail/types.hpp"
#include <cstddef>
#include <span>
#include <string_view>

namespace clap::detail {
class Parser {
  public:
	struct Result {
		Outcome outcome{};
		std::string_view command_identifier{};
	};

	explicit Parser(Context const& context, std::span<std::string_view const> args) noexcept(false);

	auto parse() noexcept(false) -> Result;

	[[nodiscard]] auto help_text() const -> std::string { return m_environment.help_text(); }

  private:
	struct Environment {
		explicit Environment(Context const& context) noexcept(false);

		[[nodiscard]] auto next_positional() -> Ptr<parameter::Positional const>;
		[[nodiscard]] auto set_command_frame(std::string_view identifier) -> bool;

		[[nodiscard]] auto help_text_main() const -> std::string;
		[[nodiscard]] auto help_text_commands() const -> std::string;
		[[nodiscard]] auto help_text_command() const -> std::string;

		[[nodiscard]] auto help_text() const -> std::string;

		PrinterWrapper printer;

		std::span<Frame const> commands{};
		std::string_view version{};
		std::string_view description{};

		Ptr<Frame const> frame{};
		std::size_t positional_index{};
	};

	void advance();

	void parse_current();
	void parse_argument();
	void parse_long_option();
	void parse_short_options();
	void parse_last_option(parameter::Named const& named, std::string_view option_lexeme);
	void parse_option_value(parameter::Named const& named, std::string_view option_lexeme);

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
