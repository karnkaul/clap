#pragma once
#include "clap/parameter.hpp"
#include "clap/printer.hpp"
#include <span>
#include <vector>

namespace clap::detail {
namespace error {
enum class Parameter : std::int8_t {
	ExtraneousPositional,
};
} // namespace error

struct PrinterWrapper {
	void printerr_prefixed(std::string_view message) const;

	IPrinter* printer{};
	std::string_view program_name{};
};

struct ParseFrame {
	void recreate(PrinterWrapper const& printer, std::span<Parameter const> parameters) noexcept(false);
	void reset(std::size_t reserve);
	[[nodiscard]] auto next_positional() -> parameter::Positional const*;

	std::vector<parameter::Named const*> named_parameters{};
	std::vector<parameter::Positional const*> positional_parameters{};
	parameter::List const* list_parameter{};
	std::size_t positional_index{};
};
} // namespace clap::detail
