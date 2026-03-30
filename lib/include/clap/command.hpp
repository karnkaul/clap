#pragma once
#include "clap/parameter.hpp"

namespace clap {
struct Command {
	explicit Command(std::string_view identifier, std::vector<Parameter> parameters, std::string_view description = {})
		: identifier(identifier), description(description), parameters(std::move(parameters)) {}

	std::string_view identifier;
	std::string_view description;
	std::vector<Parameter> parameters;
};
using CommandList = std::vector<Command>;

[[nodiscard]] inline auto command(std::string_view const identifier, std::vector<Parameter> parameters, std::string_view const description = {}) {
	return Command{identifier, std::move(parameters), description};
}
} // namespace clap
