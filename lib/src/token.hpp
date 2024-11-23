#pragma once
#include <string_view>

namespace cliq {
enum class TokenType {
	None,
	Option,	   // -[-][A-z]+[=[A-z]+]
	Argument,  // [A-z]+
	ForceArgs, // --
};

enum class OptionType {
	None,
	Letters, // -[A-z]+[=[A-z]+]
	Word,	 // --[A-z]+[=[A-z]+]
};

struct Token {
	std::string_view arg{};
	std::string_view value{};
	TokenType token_type{};
	OptionType option_type{};
};

constexpr auto is_alpha(char const c) -> bool { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }

constexpr auto to_token(std::string_view const input) -> Token {
	if (input.empty()) { return {}; }
	auto ret = Token{.arg = input, .value = input};
	if (ret.value == "--") {
		ret.value = {};
		ret.token_type = TokenType::ForceArgs;
	} else if (ret.value.starts_with("--")) {
		ret.token_type = TokenType::Option;
		ret.value = ret.value.substr(2);
		ret.option_type = OptionType::Word;
	} else if (ret.value.size() > 1 && ret.value.starts_with('-') && is_alpha(ret.value[1])) {
		ret.token_type = TokenType::Option;
		ret.value = ret.value.substr(1);
		ret.option_type = OptionType::Letters;
	} else {
		ret.token_type = TokenType::Argument;
	}
	return ret;
}
} // namespace cliq
