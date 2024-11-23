#include <scanner.hpp>
#include <array>

namespace {
using namespace clap;

static_assert([] {
	constexpr auto args = std::array{"-a=b"};
	auto scanner = Scanner{args};

	if (!scanner.next()) { return false; }
	if (scanner.get_token_type() != TokenType::Option) { return false; }
	if (scanner.get_option_type() != OptionType::Letters) { return false; }
	if (scanner.get_key() != "a") { return false; }
	if (scanner.get_value() != "b") { return false; }

	return !scanner.next();
}());

static_assert([] {
	constexpr auto args = std::array{"-a", "b"};
	auto scanner = Scanner{args};

	if (!scanner.next()) { return false; }
	if (scanner.get_token_type() != TokenType::Option) { return false; }
	if (scanner.get_option_type() != OptionType::Letters) { return false; }
	if (scanner.get_key() != "a") { return false; }
	if (!scanner.get_value().empty()) { return false; }

	if (!scanner.next()) { return false; }
	if (scanner.get_token_type() != TokenType::Argument) { return false; }
	if (scanner.get_value() != "b") { return false; }

	return !scanner.next();
}());

static_assert([] {
	constexpr auto args = std::array{"--abcd=efgh"};
	auto scanner = Scanner{args};

	if (!scanner.next()) { return false; }
	if (scanner.get_token_type() != TokenType::Option) { return false; }
	if (scanner.get_option_type() != OptionType::Word) { return false; }
	if (scanner.get_key() != "abcd") { return false; }
	if (scanner.get_value() != "efgh") { return false; }

	return !scanner.next();
}());

static_assert([] {
	constexpr auto args = std::array{"--abcd", "efgh"};
	auto scanner = Scanner{args};

	if (!scanner.next()) { return false; }
	if (scanner.get_token_type() != TokenType::Option) { return false; }
	if (scanner.get_option_type() != OptionType::Word) { return false; }
	if (scanner.get_key() != "abcd") { return false; }
	if (!scanner.get_value().empty()) { return false; }

	if (!scanner.next()) { return false; }
	if (scanner.get_value() != "efgh") { return false; }

	return !scanner.next();
}());

static_assert([] {
	constexpr auto args = std::array{"-abcd", "e"};
	auto scanner = Scanner{args};

	if (!scanner.next()) { return false; }
	if (scanner.get_token_type() != TokenType::Option) { return false; }
	if (scanner.get_option_type() != OptionType::Letters) { return false; }

	auto letter = char{};
	auto is_last = true;
	if (!scanner.next_letter(letter, is_last) || letter != 'a' || is_last) { return false; }
	if (!scanner.next_letter(letter, is_last) || letter != 'b' || is_last) { return false; }
	if (!scanner.next_letter(letter, is_last) || letter != 'c' || is_last) { return false; }
	if (!scanner.next_letter(letter, is_last) || letter != 'd' || !is_last) { return false; }
	if (!scanner.get_value().empty()) { return false; }

	if (!scanner.next()) { return false; }

	if (scanner.get_token_type() != TokenType::Argument) { return false; }
	if (scanner.get_value() != "e") { return false; }

	return !scanner.next();
}());

static_assert([] {
	constexpr auto args = std::array{"-a=b", "-c"};
	auto scanner = Scanner{args};

	scanner.next(); // skip checking a=b

	if (!scanner.next()) { return false; }
	if (scanner.get_token_type() != TokenType::Option) { return false; }
	if (scanner.get_option_type() != OptionType::Letters) { return false; }
	if (scanner.get_key() != "c") { return false; }
	if (!scanner.get_value().empty()) { return false; }

	return !scanner.next();
}());
} // namespace
