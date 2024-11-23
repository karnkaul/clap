#include <token.hpp>

namespace {
using namespace cliq;

static_assert([] {
	auto const token = to_token("--");
	return token.token_type == TokenType::ForceArgs && token.value.empty();
}());

static_assert([] {
	auto const token = to_token("foo");
	return token.token_type == TokenType::Argument && token.value == "foo";
}());

static_assert([] {
	auto const token = to_token("-bar=123");
	return token.token_type == TokenType::Option && token.option_type == OptionType::Letters && token.value == "bar=123";
}());

static_assert([] {
	auto const token = to_token("--bar=123");
	return token.token_type == TokenType::Option && token.option_type == OptionType::Word && token.value == "bar=123";
}());

static_assert([] {
	auto const token = to_token("-5");
	return token.token_type == TokenType::Argument && token.value == "-5";
}());

} // namespace
// tests
