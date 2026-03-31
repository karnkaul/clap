#pragma once
#include "clap/command.hpp"
#include "clap/parameter.hpp"
#include "clap/printer.hpp"
#include "clap/program.hpp"
#include "clap/spec.hpp"
#include <cstdint>
#include <format>
#include <span>
#include <string_view>
#include <vector>

namespace clap::detail {
enum class Error : std::int8_t {
	// internal
	UnrecognizedToken,

	// parse
	UnknownArgument,
	InvalidArgument,
	UnrecognizedOption,
	OptionRequiresArgument,
	MissingRequiredArgument,
	MissingRequiredCommand,
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
	explicit Frame(std::span<Parameter const> parameters) noexcept(false);
	explicit Frame(std::span<parameter::Named const> options) noexcept(true);

	Ptr<Command const> command{};
	std::vector<Ptr<parameter::Named const>> named_parameters{};
	std::vector<Ptr<parameter::Positional const>> positional_parameters{};
	Ptr<parameter::List const> list_parameter{};
};

class Context {
	ParameterList m_parameters{};
	OptionList m_options{};
	CommandList m_commands{};

  public:
	Context(Context const&) = delete;
	Context(Context&&) = delete;
	Context& operator=(Context&&) = delete;
	Context& operator=(Context const&) = delete;

	~Context() = default;

	explicit Context(spec::Parameters spec) noexcept(false);
	explicit Context(spec::Commands spec) noexcept(false);

	void override_program_name_if_empty(std::string_view program_name);

	PrinterWrapper printer;

	Frame main;
	std::vector<Frame> commands{};

	std::string_view version{};
	std::string_view description{};
	CommandPolicy command_policy{};
};
} // namespace clap::detail
