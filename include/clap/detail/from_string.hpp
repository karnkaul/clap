#pragma once
#include <charconv>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace clap::detail {
template <typename Type>
bool parse_string(std::string_view value, Type& out) {
	if constexpr (std::same_as<bool, Type>) {
		out = value == "true";
		return true;
	} else if constexpr (std::integral<Type> || std::floating_point<Type>) {
		auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), out);
		return ec == std::errc{} && ptr == value.data() + value.size();
	} else if constexpr (std::constructible_from<Type, std::string_view>) {
		out = Type{value};
		return true;
	} else {
		auto str = std::stringstream{std::string{value}};
		return static_cast<bool>(str >> out);
	}
}

using FromString = std::function<bool(std::string_view)>;

struct Argument {
	std::string usage{};
	FromString from_string{};
	bool required{};

	explicit operator bool() const { return !!from_string; }
};

template <typename Type>
FromString make_from_string(Type& out) {
	return [&out](std::string_view value) { return parse_string(value, out); };
}

template <typename Type>
FromString make_from_string(std::vector<Type>& out) {
	return [&out](std::string_view value) {
		auto t = Type{};
		if (parse_string(value, t)) {
			out.push_back(t);
			return true;
		}
		return false;
	};
}
} // namespace clap::detail
