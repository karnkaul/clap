#pragma once

namespace clap {
template <typename... Ts>
struct Visitor : Ts... {
	using Ts::operator()...;
};
} // namespace clap
