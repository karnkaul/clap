import clap.parser;

#include "common.hpp"
#include "klib/unit_test/unit_test.hpp"

namespace clap::test {
namespace {
using detail::ProgramEnvironment;
using detail::ProgramParser;

struct Fixture {
	auto parse(OptionList program_options, CommandList commands, std::string_view const expr) -> Result {
		auto environment = ProgramEnvironment{{}, {}, std::move(program_options), std::move(commands), policy};
		auto parser = ProgramParser{context, environment};
		auto const words = to_words(expr);
		return parser.parse(words);
	}

	MockContext context{};
	CommandPolicy policy{CommandPolicy::Required};
};

TEST_CASE(program_parser_required_command) {
	auto fixture = Fixture{};
	auto arg = std::string_view{};
	auto result = fixture.parse({},
								{
									command("run", {positional_required(arg, "ARG")}),
								},
								"run value");

	EXPECT(!result.should_early_exit());
	EXPECT(result.command_identifier() == "run");
	EXPECT(arg == "value");
}

TEST_CASE(program_parser_preserves_command_arguments) {
	auto fixture = Fixture{};

	auto arg = std::string_view{};
	auto words = std::vector<std::string_view>{};

	auto result = fixture.parse({},
								{
									command("run",
											{
												positional_required(arg, "ARG"),
												positional_list(words, "WORDS"),
											}),
								},
								"run first second third");
	EXPECT(!result.should_early_exit());
	EXPECT(arg == "first");
	ASSERT(words.size() == 2);
	EXPECT(words[0] == "second");
	EXPECT(words[1] == "third");
}

TEST_CASE(program_parser_commands_program_option) {
	auto fixture = Fixture{};

	auto global = false;
	auto arg = std::string_view{};

	auto result = fixture.parse({named_flag(global, "g,global")},
								{
									command("run", {positional_required(arg, "ARG")}),
								},
								"-g run value");
	EXPECT(!result.should_early_exit());
	EXPECT(global);
	EXPECT(arg == "value");
}

TEST_CASE(program_parser_missing_required_command) {
	auto fixture = Fixture{};

	auto result = fixture.parse({},
								{
									command("run", {}),
								},
								{});
	EXPECT(result.exit_code() == ExitCode::MissingRequiredCommand);
}

TEST_CASE(program_parser_unrecognized_command) {
	auto fixture = Fixture{};

	auto result = fixture.parse({},
								{
									command("run", {}),
								},
								"other");
	EXPECT(result.exit_code() == ExitCode::UnrecognizedCommand);
}

TEST_CASE(program_parser_optional_without_command) {
	auto fixture = Fixture{};
	fixture.policy = CommandPolicy::Optional;

	auto result = fixture.parse({},
								{
									command("run", {}),
								},
								{});
	EXPECT(!result.should_early_exit());
	EXPECT(result.command_identifier().empty());
}

TEST_CASE(program_parser_commands_missing_program_option_value) {
	auto fixture = Fixture{};

	auto i32 = int{};

	auto result = fixture.parse({named_option(i32, "i")},
								{
									command("run", {}),
								},
								"-i");
	EXPECT(result.exit_code() == ExitCode::OptionRequiresArgument);
}

TEST_CASE(program_parser_reparse) {
	auto context = MockContext{};
	auto arg = std::string_view{};
	auto environment = ProgramEnvironment{
		{},
		{},
		{},
		{
			command("run", {positional_required(arg, "ARG")}),
		},
		CommandPolicy::Required,
	};

	auto parser = ProgramParser{context, environment};

	auto words = to_words("run first");
	auto result = parser.parse(words);
	EXPECT(!result.should_early_exit());
	EXPECT(arg == "first");

	words = to_words("run second");
	result = parser.parse(words);
	EXPECT(!result.should_early_exit());
	EXPECT(arg == "second");
}
} // namespace
} // namespace clap::test
