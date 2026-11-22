import clap.parser;

#include "common.hpp"
#include "klib/unit_test/unit_test.hpp"

namespace clap::test {
namespace {
TEST_CASE(command_parser_required_command) {
	auto context = MockContext{};
	auto parser = CommandParser{context};
	auto arg = std::string_view{};

	auto result = parser.set_spec(spec::Commands{
		.commands =
			{
				command("run", {positional_required(arg, "ARG")}),
			},
	});
	EXPECT(!result.should_early_exit());

	result = parser.parse(to_words("run value"));
	EXPECT(!result.should_early_exit());
	EXPECT(result.command_identifier() == "run");
	EXPECT(arg == "value");
}

TEST_CASE(command_parser_preserves_command_arguments) {
	auto context = MockContext{};
	auto parser = CommandParser{context};
	auto arg = std::string_view{};
	auto words = std::vector<std::string_view>{};

	auto result = parser.set_spec(spec::Commands{
		.commands =
			{
				command("run",
						{
							positional_required(arg, "ARG"),
							positional_list(words, "WORDS"),
						}),
			},
	});
	EXPECT(!result.should_early_exit());

	result = parser.parse(to_words("run first second third"));
	EXPECT(!result.should_early_exit());
	EXPECT(arg == "first");
	ASSERT(words.size() == 2);
	EXPECT(words[0] == "second");
	EXPECT(words[1] == "third");
}

TEST_CASE(command_parser_program_option) {
	auto context = MockContext{};
	auto parser = CommandParser{context};
	auto global = false;
	auto arg = std::string_view{};

	auto result = parser.set_spec(spec::Commands{
		.options = {named_flag(global, "g,global")},
		.commands =
			{
				command("run", {positional_required(arg, "ARG")}),
			},
	});
	EXPECT(!result.should_early_exit());

	result = parser.parse(to_words("-g run value"));
	EXPECT(!result.should_early_exit());
	EXPECT(global);
	EXPECT(arg == "value");
}

TEST_CASE(command_parser_missing_required_command) {
	auto context = MockContext{};
	auto parser = CommandParser{context};

	auto result = parser.set_spec(spec::Commands{
		.commands =
			{
				command("run", {}),
			},
	});
	EXPECT(!result.should_early_exit());

	result = parser.parse({});
	EXPECT(result.exit_code() == ExitCode::MissingRequiredCommand);
}

TEST_CASE(command_parser_unrecognized_command) {
	auto context = MockContext{};
	auto parser = CommandParser{context};

	auto result = parser.set_spec(spec::Commands{
		.commands =
			{
				command("run", {}),
			},
	});
	EXPECT(!result.should_early_exit());

	result = parser.parse(to_words("other"));
	EXPECT(result.exit_code() == ExitCode::UnrecognizedCommand);
}

TEST_CASE(command_parser_optional_without_command) {
	auto context = MockContext{};
	auto parser = CommandParser{context};

	auto result = parser.set_spec(spec::Commands{
		.commands =
			{
				command("run", {}),
			},
		.policy = CommandPolicy::Optional,
	});
	EXPECT(!result.should_early_exit());

	result = parser.parse({});
	EXPECT(!result.should_early_exit());
	EXPECT(result.command_identifier().empty());
}

TEST_CASE(command_parser_missing_program_option_value) {
	auto context = MockContext{};
	auto parser = CommandParser{context};
	auto i32 = int{};

	auto result = parser.set_spec(spec::Commands{
		.options = {named_option(i32, "i")},
		.commands =
			{
				command("run", {}),
			},
	});
	EXPECT(!result.should_early_exit());

	result = parser.parse(to_words("-i"));
	EXPECT(result.exit_code() == ExitCode::OptionRequiresArgument);
}

TEST_CASE(command_parser_reparse) {
	auto context = MockContext{};
	auto parser = CommandParser{context};
	auto arg = std::string_view{};

	auto result = parser.set_spec(spec::Commands{
		.commands =
			{
				command("run", {positional_required(arg, "ARG")}),
			},
	});
	EXPECT(!result.should_early_exit());

	result = parser.parse(to_words("run first"));
	EXPECT(!result.should_early_exit());
	EXPECT(arg == "first");

	result = parser.parse(to_words("run second"));
	EXPECT(!result.should_early_exit());
	EXPECT(arg == "second");
}
} // namespace
} // namespace clap::test
