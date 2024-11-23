#pragma once
#include <clap/concepts.hpp>
#include <charconv>
#include <vector>

namespace cliq {
using Assignment = bool (*)(void* binding, std::string_view value);
using AsString = std::string (*)(void const* binding);

inline auto assign_to(bool& out, std::string_view /*value*/) -> bool {
	out = true;
	return true;
}

template <NumberT Type>
auto assign_to(Type& out, std::string_view const value) {
	auto const* last = value.data() + value.size();
	auto const [ptr, ec] = std::from_chars(value.data(), last, out);
	return ptr == last && ec == std::errc{};
}

template <StringyT Type>
auto assign_to(Type& out, std::string_view const value) -> bool {
	out = value;
	return true;
}

template <typename Type>
auto assign_to(std::vector<Type>& out, std::string_view const value) -> bool {
	auto t = Type{};
	if (!assign_to(t, value)) { return false; }
	out.push_back(std::move(t));
	return true;
}

template <typename Type>
auto as_string(Type const& t) -> std::string {
	if constexpr (std::constructible_from<std::string, Type>) {
		return std::string{t};
	} else {
		using std::to_string;
		return to_string(t);
	}
}

template <typename Type>
auto as_string(std::vector<Type> const& /*vec*/) -> std::string {
	return "...";
}

struct Binding {
	Assignment assign{};
	AsString to_string{};

	template <typename Type>
	static auto create() -> Binding {
		return Binding{
			.assign = [](void* binding, std::string_view const value) -> bool { return assign_to(*static_cast<Type*>(binding), value); },
			.to_string = [](void const* binding) -> std::string { return as_string(*static_cast<Type const*>(binding)); },
		};
	}
};
} // namespace cliq
