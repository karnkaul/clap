#pragma once
#include "clap/command.hpp"
#include "clap/parameter.hpp"
#include "clap/printer.hpp"
#include <format>
#include <span>
#include <vector>

namespace clap::detail {
namespace error {
enum class Parameter : std::int8_t {
	ExtraneousPositional,
};
} // namespace error

struct PrinterWrapper {
	template <typename... Args>
	void printerr_prefixed(std::format_string<Args...> fmt, Args&&... args) const {
		auto message = std::string{program_name};
		if (!message.empty()) { message += ": "; }
		std::format_to(std::back_inserter(message), fmt, std::forward<Args>(args)...);
		printer->printerr(message);
	}

	IPrinter* printer{};
	std::string_view program_name{};
};

struct ParseFrame2 {
	[[nodiscard]] static auto create(PrinterWrapper const& printer, std::span<Parameter const> parameters) noexcept(false) -> ParseFrame2;
	[[nodiscard]] static auto create(PrinterWrapper const& printer, Command const& command) noexcept(false) -> ParseFrame2;

	Command const* command{};
	std::vector<parameter::Named const*> named_parameters{};
	std::vector<parameter::Positional const*> positional_parameters{};
	parameter::List const* list_parameter{};
};

struct ParseFrame {
	void recreate(PrinterWrapper const& printer, std::span<Parameter const> parameters) noexcept(false);
	void recreate(PrinterWrapper const& printer, Command const& command) noexcept(false);
	void reset(std::size_t reserve);

	Command const* command{};
	std::vector<parameter::Named const*> named_parameters{};
	std::vector<parameter::Positional const*> positional_parameters{};
	parameter::List const* list_parameter{};
};
} // namespace clap::detail
