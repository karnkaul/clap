#include <src/parser.hpp>
#include <format>
#include <functional>
#include <iostream>
#include <source_location>

namespace {
int g_tests_failed{};
bool g_test_failed{};

using namespace clap;
using namespace detail;

struct Assertion : std::exception {
	char const* what() const noexcept final { return "Assertion"; }
};

void do_fail(std::string_view type, std::string_view expr, std::source_location const& sl) {
	std::cerr << std::format("{} failed: {}\n\t{}:{}\n", type, expr, sl.file_name(), sl.line());
	g_test_failed = true;
}

void do_expect(bool pred, std::string_view expr, std::source_location const& sl) {
	if (pred) { return; }
	do_fail("expectation", expr, sl);
}

void do_assert(bool pred, std::string_view expr, std::source_location const& sl) {
	if (pred) { return; }
	do_fail("assertion", expr, sl);
	throw Assertion{};
}

#define EXPECT(expr) do_expect(!!(expr), #expr, std::source_location::current())
#define ASSERT(expr) do_assert(!!(expr), #expr, std::source_location::current())

template <typename F>
void run_test(std::string_view name, F func) {
	g_test_failed = false;
	try {
		std::invoke(func);
	} catch (Assertion const&) {}
	if (g_test_failed) {
		++g_tests_failed;
		std::cerr << std::format("[FAILED] {}\n", name);
	}
}

#define RUN_TEST(func) run_test(#func, func)
} // namespace

namespace {
using CString = char const*;

void long_option_implicit() {
	CString const input[]{"--long-option"};
	auto parser = Parser{.input = input};
	bool long_option{};
	parser.spec.options.push_back({.key = "long-option", .out_was_passed = &long_option});

	bool res = parser.parse_next();
	EXPECT(res == true);
	EXPECT(long_option == true);
}

void long_option_explicit() {
	CString const input[]{"--long-option=false"};
	auto parser = Parser{.input = input};
	bool long_option{};
	parser.spec.options.push_back({.key = "long-option", .argument = Argument{.from_string = make_from_string(long_option)}});

	bool const res = parser.parse_next();
	EXPECT(res == true);
	EXPECT(long_option == false);
}

void positional() {
	struct {
		std::string_view first{};
		std::string_view second{};
		int third{};
	} data{};
	CString const input[]{"foo", "bar", "42"};
	auto parser = Parser{.input = input};
	parser.spec.positional = {
		Option{.key = "first", .argument = Argument{.from_string = make_from_string(data.first)}},
		Option{.key = "second", .argument = Argument{.from_string = make_from_string(data.second)}},
		Option{.key = "third", .argument = Argument{.from_string = make_from_string(data.third)}},
	};

	auto parsed = int{};
	while (parser.parse_next()) { ++parsed; }
	EXPECT(parsed == 3);
	EXPECT(data.first == "foo");
	EXPECT(data.second == "bar");
	EXPECT(data.third == 42);
}

void short_options() {
	struct {
		int foo{};
		bool bar{};
		bool verbose{};
	} data{};
	CString const input[]{"-vbf=42"};
	auto parser = Parser{.input = input};
	parser.spec.options = {
		Option{.key = "foo", .letter = 'f', .argument = Argument{.from_string = make_from_string(data.foo)}},
		Option{.key = "bar", .letter = 'b', .out_was_passed = &data.bar},
		Option{.key = "verbose", .letter = 'v', .out_was_passed = &data.verbose},
	};

	bool res = parser.parse_next();
	EXPECT(res == true);
	EXPECT(data.foo == 42);
	EXPECT(data.bar == true);
	EXPECT(data.verbose == true);
}

void short_concat() {
	struct {
		int foo{};
		bool bar{};
		bool verbose{};
	} data{};
	CString const input[]{"-vbf42"};
	auto parser = Parser{.input = input};
	parser.spec.options = {
		Option{.key = "foo", .letter = 'f', .argument = Argument{.from_string = make_from_string(data.foo)}},
		Option{.key = "bar", .letter = 'b', .out_was_passed = &data.bar},
		Option{.key = "verbose", .letter = 'v', .out_was_passed = &data.verbose},
	};
	bool res = parser.parse_next();
	EXPECT(res == true);
	EXPECT(data.foo == 42);
	EXPECT(data.bar == true);
	EXPECT(data.verbose == true);
}

void short_spaced() {
	struct {
		int foo{};
		bool bar{};
		bool verbose{};
	} data{};
	CString const input[]{"-vbf", "42"};
	auto parser = Parser{.input = input};
	parser.spec.options = {
		Option{.key = "foo", .letter = 'f', .argument = Argument{.from_string = make_from_string(data.foo)}},
		Option{.key = "bar", .letter = 'b', .out_was_passed = &data.bar},
		Option{.key = "verbose", .letter = 'v', .out_was_passed = &data.verbose},
	};

	bool res = parser.parse_next();
	EXPECT(res == true);
	EXPECT(data.foo == 42);
	EXPECT(data.bar == true);
	EXPECT(data.verbose == true);
}

void long_positional() {
	struct {
		std::string first{};
		std::string second{};
		int third{};
		bool verbose{};
	} data{};
	CString const input[]{"foo", "bar", "--verbose", "42"};
	auto parser = Parser{.input = input};
	parser.spec.options.push_back({.key = "verbose", .out_was_passed = &data.verbose});
	parser.spec.positional = {
		Option{.argument = Argument{.from_string = make_from_string(data.first)}},
		Option{.argument = Argument{.from_string = make_from_string(data.second)}},
		Option{.argument = Argument{.from_string = make_from_string(data.third)}},
	};

	auto parsed = int{};
	while (parser.parse_next()) { ++parsed; }
	EXPECT(parsed == 4);
	EXPECT(data.first == input[0]);
	EXPECT(data.second == input[1]);
	EXPECT(data.third == 42);
	EXPECT(data.verbose == true);
}

void missing_required_arg() {
	CString const input[]{"--long-option"};
	auto parser = Parser{.input = input};
	bool long_option{};
	parser.spec.options.push_back(Option{.key = "long-option", .argument = Argument{.from_string = make_from_string(long_option), .required = true}});

	bool error_caught{};
	try {
		parser.parse_next();
	} catch (Error const& error) {
		EXPECT(std::string_view{error.what()} == Parser::argument_required(parser.spec.options[0].key).what());
		error_caught = true;
	}
	EXPECT(error_caught);
}

void unexpected_arg() {
	CString const input[]{"--long-option=foobar"};
	auto parser = Parser{.input = input};
	parser.spec.options.push_back({.key = "long-option"});

	bool error_caught{};
	try {
		parser.parse_next();
	} catch (Error const& error) {
		EXPECT(std::string_view{error.what()} == Parser::unexpected_argument(parser.spec.options[0].key).what());
		error_caught = true;
	}
	EXPECT(error_caught);
}

void invalid_value() {
	CString const input[]{"-ofoobar"};
	auto parser = Parser{.input = input};
	auto foobar = int{};
	parser.spec.options.push_back({.key = "option", .letter = 'o', .argument = Argument{.from_string = make_from_string(foobar)}});

	bool error_caught{};
	try {
		parser.parse_next();
	} catch (Error const& error) {
		EXPECT(std::string_view{error.what()} == Parser::invalid_value(parser.spec.options[0].key, "foobar").what());
		error_caught = true;
	}
	EXPECT(error_caught);
}

void full_parse() {
	struct {
		std::string command{};
		int optimization{};
		int level{};
		bool verbose{};
		std::string fsanitize{};
		std::vector<std::string> defines{};
		std::vector<std::string> files{};
	} data{};
	CString const input[]{"compile", "-vl=3", "-DFOO", "-DBAR", "-O2", "--fsanitize=address", "a.cpp", "b.cpp"};
	auto parser = Parser{.input = input};
	parser.spec.options = {
		{.key = "verbose", .letter = 'v', .out_was_passed = &data.verbose},
		{.key = "level", .letter = 'l', .argument = Argument{.from_string = make_from_string(data.level), .required = true}},
		{.key = "define", .letter = 'D', .argument = Argument{.from_string = make_from_string(data.defines), .required = true}},
		{.key = "optimization", .letter = 'O', .argument = Argument{.from_string = make_from_string(data.optimization), .required = true}},
		{.key = "fsanitize", .argument = Argument{.from_string = make_from_string(data.fsanitize), .required = true}},
	};
	parser.spec.positional = {
		{.key = "compile", .argument = Argument{.from_string = make_from_string(data.command)}},
	};
	parser.spec.unmatched.argument = Argument{.from_string = make_from_string(data.files)};

	while (parser.parse_next())
		;

	EXPECT(data.verbose == true);
	EXPECT(data.level == 3);
	EXPECT(data.optimization == 2);
	EXPECT(data.fsanitize == "address");
	ASSERT(data.defines.size() == 2);
	EXPECT(data.defines[0] == "FOO");
	EXPECT(data.defines[1] == "BAR");
	ASSERT(data.files.size() == 2);
	EXPECT(data.files[0] == "a.cpp");
	EXPECT(data.files[1] == "b.cpp");
}

} // namespace

int main() {
	try {
		RUN_TEST(long_option_implicit);
		RUN_TEST(long_option_explicit);
		RUN_TEST(short_options);
		RUN_TEST(short_concat);
		RUN_TEST(short_spaced);
		RUN_TEST(positional);
		RUN_TEST(long_positional);
		RUN_TEST(missing_required_arg);
		RUN_TEST(unexpected_arg);
		RUN_TEST(invalid_value);
		RUN_TEST(full_parse);

		if (g_tests_failed > 0) {
			std::cerr << std::format("{} tests failed\n", g_tests_failed);
			return EXIT_FAILURE;
		}
	} catch (Error const& error) {
		std::cerr << "Parse error: " << error.what() << "\n";
		return EXIT_FAILURE;
	}

	std::cout << "all tests passed\n";
}
