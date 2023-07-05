#pragma once
#include <clap/clap.hpp>
#include <format>
#include <span>

namespace clap {
struct Error : std::runtime_error {
	template <typename... Args>
	explicit Error(std::format_string<Args...> fmt, Args&&... args) : std::runtime_error{std::format(fmt, std::forward<Args>(args)...)} {}
};

struct Option {
	std::string key{};
	char letter{};
	std::string description{};
	detail::Argument argument{};
	bool* out_was_passed{};

	void parse_argument(std::string_view value) const;
};

struct OptionSpec {
	std::vector<Option> positional{};
	std::vector<Option> options{};
	Option unmatched{};
};

/*
1: -abc=foo
	^^^ ~~~
2: -abc foo
	^^^ ~~~
3: -abcfoo
	^^^~~~
4: --long=foo
	 ^    ~~~
5: --long foo
	 ^    ~~~
*/

struct Parser {
	static Error invalid_option(std::string_view option);
	static Error unrecognized_argument(std::string_view argument);
	static Error unexpected_argument(std::string_view option);
	static Error argument_required(std::string_view option);
	static Error invalid_value(std::string_view option, std::string_view value);

	std::span<char const* const> input{};
	OptionSpec spec{};

	std::string_view current{};
	bool force_positional{};
	std::size_t position{};

	bool parse_next();

	void argument();
	void option();

	void long_option();
	void short_options();

	bool at_end() const { return input.empty(); }
	std::string_view peek() const { return at_end() ? std::string_view{} : input[0]; }
	bool advance();
};
} // namespace clap
