#pragma once
#include "clap/command.hpp"
#include "clap/parameter.hpp"
#include "clap/printer.hpp"
#include "clap/program.hpp"
#include <cstdint>

namespace clap {
enum class CommandPolicy : std::int8_t { Required, Optional };

namespace spec {
struct Parameters {
	/// \brief Each parameter can be named or positional, the last positional may be optional or a list.
	ParameterList parameters{};

	Program program{};
	IPrinter* custom_printer{};
};

struct Commands {
	/// \brief Only options (named parameters) are supported with commands.
	OptionList options{};
	/// \brief One or more Commands each with their own ParameterLists.
	CommandList commands{};
	/// \brief Whether a command selection is required.
	CommandPolicy policy{CommandPolicy::Required};

	Program program{};
	IPrinter* custom_printer{};
};
} // namespace spec
} // namespace clap
