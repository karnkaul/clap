#pragma once
#include "clap/parameter.hpp"
#include <string_view>

namespace clap {
struct Command {
	explicit Command(std::string_view identifier, ParameterList parameters, std::string_view description, std::string_view epilogue)
		: identifier(identifier), description(description), parameters(std::move(parameters)), epilogue(epilogue) {}

	std::string_view identifier;
	std::string_view description;
	ParameterList parameters;
	std::string_view epilogue;
};
using CommandList = std::vector<Command>;

[[nodiscard]] auto command(std::string_view identifier, ParameterList parameters, std::string_view description = {}, std::string_view epilogue = {}) -> Command;
} // namespace clap
