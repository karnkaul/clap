#include "detail/scanner.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <array>
#include <span>
#include <string_view>

namespace {
using namespace std::string_view_literals;

using namespace clap::detail;

TEST_CASE(scanner_empty) {
	auto scanner = Scanner{{}};
	auto token = Token{};
	EXPECT(!scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Eof);
}

TEST_CASE(scanner_strings) {
	static constexpr auto words = std::array{
		"foo"sv,
		"bar"sv,
		"fubar"sv,
	};
	auto scanner = Scanner{words};
	auto token = Token{};
	for (auto const word : words) {
		EXPECT(scanner.scan_next(token));
		EXPECT(token.type == Token::Type::String);
		EXPECT(word == token.lexeme);
	}
	EXPECT(!scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Eof);
}

TEST_CASE(scanner_mm_string) {
	static constexpr auto words = std::array{
		"--foo"sv,
		"--bar"sv,
		"--fubar"sv,
	};
	auto scanner = Scanner{words};
	auto token = Token{};
	for (auto const word : words) {
		EXPECT(scanner.scan_next(token));
		EXPECT(token.type == Token::Type::MinusMinusString);
		EXPECT(word == token.lexeme);
	}
	EXPECT(!scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Eof);
}

TEST_CASE(scanner_m_string) {
	static constexpr auto words = std::array{
		"-foo"sv,
		"-b"sv,
		"-fubar"sv,
	};
	auto scanner = Scanner{words};
	auto token = Token{};
	for (auto const word : words) {
		EXPECT(scanner.scan_next(token));
		EXPECT(token.type == Token::Type::MinusString);
		EXPECT(word == token.lexeme);
	}
	EXPECT(!scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Eof);
}

TEST_CASE(scanner_mm_string_value) {
	static constexpr auto words = std::array{
		"--foo=bar"sv,
		"--fubar=42"sv,
	};
	auto scanner = Scanner{words};
	auto token = Token{};

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::MinusMinusString);
	EXPECT(token.lexeme == "--foo");
	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Equals);
	EXPECT(token.lexeme == "=");
	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::String);
	EXPECT(token.lexeme == "bar");

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::MinusMinusString);
	EXPECT(token.lexeme == "--fubar");
	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Equals);
	EXPECT(token.lexeme == "=");
	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::String);
	EXPECT(token.lexeme == "42");

	EXPECT(!scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Eof);
}

TEST_CASE(scanner_m_string_value) {
	static constexpr auto words = std::array{
		"-foo=bar"sv,
		"-fubar=42"sv,
	};
	auto scanner = Scanner{words};
	auto token = Token{};

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::MinusString);
	EXPECT(token.lexeme == "-foo");
	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Equals);
	EXPECT(token.lexeme == "=");
	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::String);
	EXPECT(token.lexeme == "bar");

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::MinusString);
	EXPECT(token.lexeme == "-fubar");
	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Equals);
	EXPECT(token.lexeme == "=");
	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::String);
	EXPECT(token.lexeme == "42");

	EXPECT(!scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Eof);
}

TEST_CASE(scanner_all) {
	static constexpr auto words = std::array{
		"-f"sv, "cmd"sv, "--flag"sv, "-fabc"sv, "-fabc=42"sv, "arg"sv, "--"sv, "-arg"sv,
	};
	auto scanner = Scanner{words};
	auto token = Token{};

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::MinusString);
	EXPECT(token.lexeme == "-f");

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::String);
	EXPECT(token.lexeme == "cmd");

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::MinusMinusString);
	EXPECT(token.lexeme == "--flag");

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::MinusString);
	EXPECT(token.lexeme == "-fabc");

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::MinusString);
	EXPECT(token.lexeme == "-fabc");

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Equals);
	EXPECT(token.lexeme == "=");

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::String);
	EXPECT(token.lexeme == "42");

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::String);
	EXPECT(token.lexeme == "arg");

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::MinusMinus);
	EXPECT(token.lexeme == "--");

	EXPECT(scanner.scan_next(token));
	EXPECT(token.type == Token::Type::MinusString);
	EXPECT(token.lexeme == "-arg");

	EXPECT(!scanner.scan_next(token));
	EXPECT(token.type == Token::Type::Eof);
}
} // namespace
