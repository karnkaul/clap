#include <src/parser.hpp>
#include <algorithm>
#include <cassert>

namespace clap {
namespace {
Option const* find_option(OptionSpec const& in, std::string_view const word) {
	if (auto it = std::ranges::find_if(in.options, [word](Option const& o) { return o.key == word; }); it != in.options.end()) { return &*it; }
	return {};
}

Option const* find_option(OptionSpec const& in, char const letter) {
	if (auto it = std::ranges::find_if(in.options, [letter](Option const& o) { return o.letter == letter; }); it != in.options.end()) { return &*it; }
	return {};
}

constexpr bool is_argument(std::string_view str) { return !str.empty() && str[0] != '-'; }
} // namespace

void Option::parse_argument(std::string_view value) const {
	if (!argument) { throw Parser::unexpected_argument(key); }
	assert(argument.from_string);
	if (!argument.from_string(value)) { throw Parser::invalid_value(key, value); }
	if (out_was_passed) { *out_was_passed = true; }
}

auto Parser::invalid_option(std::string_view option) -> Error { return Error{"invalid option -- '{}'", option}; }
auto Parser::unrecognized_argument(std::string_view argument) -> Error { return Error{"unrecognized argument: {}", argument}; }
auto Parser::unexpected_argument(std::string_view option) -> Error { return Error{"option does not allow an argument -- '{}'", option}; }
auto Parser::argument_required(std::string_view option) -> Error { return Error{"option requires an argument -- '{}'", option}; }
auto Parser::invalid_value(std::string_view option, std::string_view value) -> Error { return Error{"invalid {}: '{}'", option, value}; }

bool Parser::parse_next() {
	if (!advance()) { return false; }
	switch (current[0]) {
	case '-': {
		current = current.substr(1);
		if (current.empty()) { return advance(); }
		if (current == "-") {
			force_positional = true;
			return true;
		}
		option();
		return true;
	}
	default: argument(); return true;
	}
}

void Parser::argument() {
	assert(!current.empty());
	if (position < spec.positional.size()) {
		auto const& option = spec.positional[position++];
		option.parse_argument(current);
	} else {
		if (!spec.unmatched.argument) { throw unrecognized_argument(current); }
		spec.unmatched.parse_argument(current);
	}
}

void Parser::option() {
	if (force_positional) { return argument(); }
	assert(!current.empty());
	switch (current[0]) {
	case '-': long_option(); break;
	default: short_options(); break;
	}
}

void Parser::long_option() {
	assert(!current.empty() && current[0] == '-');
	current = current.substr(1);
	if (current.empty()) { return; }
	auto arg = std::string_view{};
	auto key = current;
	if (auto const i = key.find('='); i != std::string_view::npos) {
		arg = key.substr(i + 1);
		key = key.substr(0, i);
	}
	auto const* option = find_option(spec, key);
	if (!option) { throw invalid_option(key); }

	if (!option->argument && !arg.empty()) { throw unexpected_argument(option->key); }

	if (option->argument) {
		bool const next_is_argument = is_argument(peek());
		if (option->argument.required && arg.empty() && !next_is_argument) { throw argument_required(option->key); }
		if (arg.empty() && next_is_argument) {
			advance();
			arg = current;
		}

		if (!arg.empty()) { option->parse_argument(arg); }
	}

	if (option->out_was_passed) { *option->out_was_passed = true; }
}

void Parser::short_options() {
	auto keys = current;
	while (keys.size() > 1) {
		char const letter = keys.front();
		assert(keys.size() > 1);
		char const next_letter = keys[1];
		bool const next_is_equal_to = next_letter == '=';
		auto const* option = find_option(spec, letter);
		if (!option) { throw invalid_option({&letter, 1}); }
		if (next_is_equal_to && !option->argument) { throw unexpected_argument(keys.substr(2)); }
		if (option->argument) {
			if (next_is_equal_to) {
				auto const arg = keys.substr(2);
				option->parse_argument(arg);
				return;
			}
			if (!find_option(spec, next_letter)) {
				// consume rest as argument
				auto const arg = keys.substr(1);
				option->parse_argument(arg);
				return;
			}
			if (option->argument.required) { throw argument_required({&letter, 1}); }
		}

		if (option->out_was_passed) { *option->out_was_passed = true; }
		keys = keys.substr(1);
	}

	assert(keys.size() == 1);
	auto const last_letter = keys.front();
	auto const* option = find_option(spec, last_letter);
	if (!option) { throw invalid_option({&last_letter, 1}); }
	if (option->argument) {
		bool const next_is_argument = is_argument(peek());
		if (option->argument.required && !next_is_argument) { throw argument_required({&last_letter, 1}); }
		if (next_is_argument) {
			advance();
			option->parse_argument(current);
			return;
		}
	}

	if (option->out_was_passed) { *option->out_was_passed = true; }
}

bool Parser::advance() {
	current = {};
	if (input.empty()) { return false; }
	current = input.front();
	input = input.subspan(1);
	return true;
}
} // namespace clap
