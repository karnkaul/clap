#pragma once
#include <string_view>

namespace clap {
struct Program {
	std::string_view name{};
	std::string_view version{};
	std::string_view description{};
};
} // namespace clap
