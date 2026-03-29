#pragma once
#include <charconv>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace clap {
namespace parameter {
template <typename T>
concept FlagT = std::same_as<T, bool>;

template <typename T>
concept NumberT = !FlagT<T> && (std::integral<T> || std::floating_point<T>);

template <typename T>
concept StringT = !FlagT<T> && (std::same_as<T, std::string> || std::same_as<T, std::string_view>);

template <typename T>
concept ValueT = NumberT<T> || StringT<T>;

enum class Type : std::int8_t { Required, Optional };

void split_named_key(std::string_view key, char& out_letter, std::string_view& out_word);

[[nodiscard]] auto parse_to(bool& out, std::string_view input) -> bool;

template <NumberT T>
[[nodiscard]] auto parse_to(T& out, std::string_view const input) -> bool {
	auto const* end = input.data() + input.size();
	auto [ptr, ec] = std::from_chars(input.data(), end, out);
	return ec == std::errc{} && ptr == end;
}

template <StringT T>
[[nodiscard]] auto parse_to(T& out, std::string_view const input) -> bool {
	if (input.empty()) { return false; }
	out = input;
	return true;
}

template <ValueT T>
[[nodiscard]] auto parse_to(std::vector<T>& out, std::string_view const input) -> bool {
	auto t = T{};
	if (!parse_to(t, input)) { return false; }
	out.push_back(std::move(t));
	return true;
}

struct Parse {
	auto operator()(std::string_view input) const -> bool;

	std::function<bool(std::string_view)> func{};
	bool* was_set{};
};

template <typename T>
[[nodiscard]] auto create_parse(T& out, bool* was_set) -> Parse {
	if (was_set) { *was_set = false; }
	return Parse{.func = [&out](std::string_view const input) { return parse_to(out, input); }, .was_set = was_set};
}

struct Named {
	template <typename T>
		requires(ValueT<T> || FlagT<T>)
	explicit Named(T& out, std::string_view const key, std::string_view const description, bool* was_set)
		: description(description), parse(create_parse(out, was_set)), is_flag(FlagT<T>) {
		split_named_key(key, letter, word);
	}

	char letter{};
	std::string_view word{};
	std::string_view description;
	Parse parse;
	bool is_flag;
};

struct Positional {
	template <ValueT T>
	explicit Positional(T& out, std::string_view const name, std::string_view const description, Type const type, bool* was_set)
		: name(name), description(description), type(type), parse(create_parse(out, was_set)) {}

	std::string_view name;
	std::string_view description;
	Type type;
	Parse parse;
};

struct List {
	template <ValueT T>
	explicit List(std::vector<T>& out, std::string_view const name, std::string_view const description, bool* was_set)
		: name(name), description(description), parse(create_parse(out, was_set)) {}

	std::string_view name;
	std::string_view description;
	Parse parse;
};
} // namespace parameter

using Parameter = std::variant<parameter::Named, parameter::Positional, parameter::List>;

template <parameter::FlagT T>
// NOLINTNEXTLINE(readability-non-const-parameter)
[[nodiscard]] auto named_flag(T& out, std::string_view const key, std::string_view const description = {}, bool* was_set = {}) {
	return Parameter{parameter::Named{out, key, description, was_set}};
}

template <parameter::ValueT T>
// NOLINTNEXTLINE(readability-non-const-parameter)
[[nodiscard]] auto named_option(T& out, std::string_view const key, std::string_view const description = {}, bool* was_set = {}) {
	return Parameter{parameter::Named{out, key, description, was_set}};
}

template <parameter::ValueT T>
// NOLINTNEXTLINE(readability-non-const-parameter)
[[nodiscard]] auto positional_required(T& out, std::string_view const name, std::string_view const description = {}, bool* was_set = {}) {
	return Parameter{parameter::Positional{out, name, description, parameter::Type::Required, was_set}};
}

template <parameter::ValueT T>
// NOLINTNEXTLINE(readability-non-const-parameter)
[[nodiscard]] auto positional_optional(T& out, std::string_view const name, std::string_view const description = {}, bool* was_set = {}) {
	return Parameter{parameter::Positional{out, name, description, parameter::Type::Optional, was_set}};
}

template <parameter::ValueT T>
// NOLINTNEXTLINE(readability-non-const-parameter)
[[nodiscard]] auto positional_list(std::vector<T>& out, std::string_view const name, std::string_view const description = {}, bool* was_set = {}) {
	return Parameter{parameter::List{out, name, description, was_set}};
}
} // namespace clap
