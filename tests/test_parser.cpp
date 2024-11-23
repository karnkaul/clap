#include <ktest/ktest.hpp>
#include <parser.hpp>
#include <array>

namespace {
using namespace clap;

constexpr auto app_info_v = AppInfo{};

TEST(parser_empty) {
	auto parser = Parser{app_info_v, {}, {}};
	auto const result = parser.parse({});
	EXPECT(!result.early_return());
	EXPECT(result.get_command_name().empty());
}

TEST(parser_values) {
	auto run = [](std::span<char const* const> cli_args) {
		auto parser = Parser{app_info_v, {}, cli_args};
		auto a = std::string_view{};
		auto bar = 0;
		auto args = std::array{
			Arg{a, "a"},
			Arg{bar, "bar"},
		};
		auto const result = parser.parse(args);
		EXPECT(!result.early_return());
		EXPECT(result.get_command_name().empty());
		EXPECT(a == "foo");
		EXPECT(bar == 42);
	};

	run(std::array{"-a", "foo", "--bar", "42"});
	run(std::array{"-a=foo", "--bar=42"});
}

TEST(parser_options) {
	auto run = [](std::span<char const* const> cli_args) {
		auto parser = Parser{app_info_v, {}, cli_args};
		auto a = false;
		auto b = false;
		auto c = 0;
		auto d = false;
		auto const args = std::array{
			Arg{a, "a,alpha"},
			Arg{b, "b,beta"},
			Arg{c, "c,charlie"},
			Arg{d, "d,delta"},
		};
		auto const result = parser.parse(args);
		EXPECT(!result.early_return());
		EXPECT(result.get_command_name().empty());
		EXPECT(a && b);
		EXPECT(c == 42);
	};

	run(std::array{"-abc", "42"});
	run(std::array{"-abc=42"});
	run(std::array{"-abd", "--charlie=42"});
	run(std::array{"--charlie=42", "-abd"});
	run(std::array{"--charlie=42", "-a", "-b", "-d"});
}

TEST(parser_positionals) {
	static constexpr auto cli_args = std::array{"42", "-5", "fubar"};
	auto parser = Parser{app_info_v, {}, cli_args};
	int a{};
	int b{};
	std::string_view c{};
	auto const args = std::array{
		Arg{a, ArgType::Required, "a"},
		Arg{b, ArgType::Required, "b"},
		Arg{c, ArgType::Required, "c"},
	};
	auto const result = parser.parse(args);
	EXPECT(!result.early_return());
	EXPECT(result.get_command_name().empty());
	EXPECT(a == 42);
	EXPECT(b == -5);
	EXPECT(c == "fubar");
}

TEST(parser_options_positionals) {
	static constexpr auto cli_args = std::array{"-ab", "42", "-c", "42", "-5", "foo", "-d=bar"};
	bool a{};
	bool b{};
	int forty_two{};
	int c{};
	int minus_five{};
	std::string_view foo{};
	std::string_view d{};
	auto const args = std::array{
		Arg{a, "a"},
		Arg{b, "b"},
		Arg{c, "c"},
		Arg{d, "d"},
		Arg{forty_two, ArgType::Required, "42"},
		Arg{minus_five, ArgType::Required, "-5"},
		Arg{foo, ArgType::Required, "foo"},
	};
	auto parser = Parser{app_info_v, {}, cli_args};
	auto const result = parser.parse(args);
	EXPECT(!result.early_return());
	EXPECT(result.get_command_name().empty());
	EXPECT(a && b && c == 42 && forty_two == 42 && minus_five == -5 && d == "bar");
}

TEST(parser_command) {
	static constexpr auto cli_args = std::array{"--app-flag", "cmd", "--cmd-flag", "cmd-arg"};
	bool cmd_flag{};
	std::string_view cmd_arg{};
	auto const cmd_args = std::array{
		Arg{cmd_flag, "cmd-flag"},
		Arg{cmd_arg, ArgType::Required, "cmd-arg"},
	};
	bool app_flag{};
	auto const app_args = std::array{
		Arg{app_flag, "app-flag"},
		Arg{cmd_args, "cmd"},
	};
	auto parser = Parser{app_info_v, {}, cli_args};
	auto const result = parser.parse(app_args);
	EXPECT(!result.early_return());
	EXPECT(app_flag == true);
	EXPECT(result.get_command_name() == "cmd");
	EXPECT(cmd_flag == true);
	EXPECT(cmd_arg == "cmd-arg");
}
} // namespace
