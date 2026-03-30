#pragma once
#include <stdexcept>

namespace clap {
struct InvalidParameterException : std::runtime_error {
	using std::runtime_error::runtime_error;
};
} // namespace clap
