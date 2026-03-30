#include "detail/parser.hpp"
#include "klib/unit_test/unit_test.hpp"

namespace {
using namespace clap;

auto get_result(std::span<Parameter const> parameters, std::span<Command const> commands, std::vector<std::string_view> const& args) {
	auto parse_input = detail::Input{
		.parameters = parameters,
		.commands = commands,
		.program = Program{.name = "clap-test"},
	};
	auto parse_context = detail::Context{parse_input};
	auto parser = detail::Parser{parse_context, args};
	return parser.parse();
}

TEST_CASE(parser_commands) {
	auto flag = bool{};
	auto const parameters = std::array{
		named_flag(flag, "f,flag"),
	};

	auto cmd_flag = bool{};
	auto cmd_arg = std::string_view{};
	auto cmd_parameters = std::vector{
		named_flag(cmd_flag, "f,flag"),
		positional_required(cmd_arg, "arg"),
	};

	auto const commands = std::array{
		command("cmd", std::move(cmd_parameters)),
	};

	auto result = get_result(parameters, commands, {"-f", "cmd", "foo"});
	EXPECT(result.outcome == Outcome::Continue);
	EXPECT(result.command_identifier == commands[0].identifier);
	EXPECT(flag);
	EXPECT(!cmd_flag);
	EXPECT(cmd_arg == "foo");
}
} // namespace
