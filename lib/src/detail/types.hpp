#pragma once
#include "clap/command.hpp"
#include "clap/parameter.hpp"
#include "clap/printer.hpp"
#include "clap/program.hpp"
#include <cstdint>
#include <format>
#include <span>
#include <string_view>
#include <vector>

namespace clap::detail {
enum class Outcome : std::int8_t { Continue, EarlyExit };

enum class Error : std::int8_t {
	// internal
	UnrecognizedToken,

	// parameter
	ExtraneousPositional,

	// parse
	UnknownArgument,
	InvalidArgument,
	UnrecognizedOption,
	OptionRequiresArgument,
	MissingRequiredArgument,
	UnexpectedToken,
};

template <typename T>
using Ptr = T*;

struct PrinterWrapper {
	explicit PrinterWrapper(Ptr<IPrinter> printer, std::string_view program_name) noexcept(true);

	template <typename... Args>
	void prefixed_err(std::format_string<Args...> fmt, Args&&... args) const {
		auto message = std::string{program_name};
		if (!message.empty()) { message += ": "; }
		std::format_to(std::back_inserter(message), fmt, std::forward<Args>(args)...);
		printer->printerr(message);
	}

	auto operator->() const { return printer; }

	Ptr<IPrinter> printer{};
	std::string_view program_name{};
};

struct Frame {
	explicit Frame(PrinterWrapper const& printer, std::span<Parameter const> parameters) noexcept(false);

	Ptr<Command const> command{};
	std::vector<Ptr<parameter::Named const>> named_parameters{};
	std::vector<Ptr<parameter::Positional const>> positional_parameters{};
	Ptr<parameter::List const> list_parameter{};
};

struct Input {
	std::span<Parameter const> parameters{};
	std::span<Command const> commands{};
	Program program{};
	Ptr<IPrinter> printer{};
};

struct Context {
	explicit Context(Input const& input) noexcept(false);

	PrinterWrapper printer;

	Frame main;
	std::vector<Frame> commands{};

	std::string_view version{};
	std::string_view description{};
};
} // namespace clap::detail
