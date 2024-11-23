#pragma once
#include <clap/app_info.hpp>
#include <clap/result.hpp>
#include <scanner.hpp>

namespace cliq {
class Parser {
  public:
	explicit Parser(AppInfo const& info, std::string_view const exe_name, std::span<char const* const> cli_args)
		: m_info(info), m_exe_name(exe_name), m_scanner(cli_args) {}

	[[nodiscard]] auto parse(std::span<Arg const> args) -> Result;

  private:
	struct Cursor {
		ParamCommand const* cmd{};
		std::size_t next_pos{};
	};

	auto select_command() -> Result;
	auto parse_next() -> Result;
	auto parse_option() -> Result;
	auto parse_letters() -> Result;
	auto parse_word() -> Result;
	auto parse_last_option(ParamOption const& option, std::string_view input) -> Result;
	auto parse_argument() -> Result;
	auto parse_positional() -> Result;

	[[nodiscard]] auto try_builtin(std::string_view word) const -> bool;
	[[nodiscard]] auto find_option(char letter) const -> ParamOption const*;
	[[nodiscard]] auto find_option(std::string_view word) const -> ParamOption const*;
	[[nodiscard]] auto find_command(std::string_view name) const -> ParamCommand const*;

	[[nodiscard]] auto next_positional() -> ParamPositional const*;

	[[nodiscard]] auto check_required() -> Result;

	[[nodiscard]] auto get_cmd_name() const -> std::string_view { return m_cursor.cmd == nullptr ? "" : m_cursor.cmd->name; }
	[[nodiscard]] auto get_help_text() const -> std::string_view { return m_cursor.cmd == nullptr ? m_info.help_text : m_cursor.cmd->help_text; }

	AppInfo const& m_info;
	std::string_view m_exe_name;

	Scanner m_scanner;
	std::span<Arg const> m_args{};
	Cursor m_cursor{};
	bool m_has_commands{};
};
} // namespace cliq
