import clap.parser;

#include "common.hpp"
#include "klib/unit_test/unit_test.hpp"

namespace clap::test {
namespace {
TEST_CASE(command_parser_required_command) {
	auto context = MockContext{};
	auto parser = CommandParser{context};
	auto arg = std::string_view{};

	auto result = parser.set_parameters({},
										{
											command("run", {positional_required(arg, "ARG")}),
										},
										CommandPolicy::Required);
	EXPECT(!result.should_early_exit());

	auto words = to_words("run value");
	auto scanner = Scanner{words};
	result = parser.parse(scanner);
	EXPECT(!result.should_early_exit());
	EXPECT(result.command_identifier() == "run");
	EXPECT(arg == "value");
}

TEST_CASE(command_parser_preserves_command_arguments) {
	auto context = MockContext{};
	auto parser = CommandParser{context};
	auto arg = std::string_view{};
	auto words = std::vector<std::string_view>{};

	auto result = parser.set_parameters({},
										{
											command("run",
													{
														positional_required(arg, "ARG"),
														positional_list(words, "WORDS"),
													}),
										},
										CommandPolicy::Required);
	EXPECT(!result.should_early_exit());

	auto const input = to_words("run first second third");
	auto scanner = Scanner{input};
	result = parser.parse(scanner);
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

	auto result = parser.set_parameters({named_flag(global, "g,global")},
										{
											command("run", {positional_required(arg, "ARG")}),
										},
										CommandPolicy::Required);
	EXPECT(!result.should_early_exit());

	auto words = to_words("-g run value");
	auto scanner = Scanner{words};
	result = parser.parse(scanner);
	EXPECT(!result.should_early_exit());
	EXPECT(global);
	EXPECT(arg == "value");
}

TEST_CASE(command_parser_missing_required_command) {
	auto context = MockContext{};
	auto parser = CommandParser{context};

	auto result = parser.set_parameters({},
										{
											command("run", {}),
										},
										CommandPolicy::Required);
	EXPECT(!result.should_early_exit());

	auto scanner = Scanner{{}};
	result = parser.parse(scanner);
	EXPECT(result.exit_code() == ExitCode::MissingRequiredCommand);
}

TEST_CASE(command_parser_unrecognized_command) {
	auto context = MockContext{};
	auto parser = CommandParser{context};

	auto result = parser.set_parameters({},
										{
											command("run", {}),
										},
										CommandPolicy::Required);
	EXPECT(!result.should_early_exit());

	auto words = to_words("other");
	auto scanner = Scanner{words};
	result = parser.parse(scanner);
	EXPECT(result.exit_code() == ExitCode::UnrecognizedCommand);
}

TEST_CASE(command_parser_optional_without_command) {
	auto context = MockContext{};
	auto parser = CommandParser{context};

	auto result = parser.set_parameters({},
										{
											command("run", {}),
										},
										CommandPolicy::Optional);
	EXPECT(!result.should_early_exit());

	auto scanner = Scanner{{}};
	result = parser.parse(scanner);
	EXPECT(!result.should_early_exit());
	EXPECT(result.command_identifier().empty());
}

TEST_CASE(command_parser_missing_program_option_value) {
	auto context = MockContext{};
	auto parser = CommandParser{context};
	auto i32 = int{};

	auto result = parser.set_parameters({named_option(i32, "i")},
										{
											command("run", {}),
										},
										CommandPolicy::Required);
	EXPECT(!result.should_early_exit());

	auto words = to_words("-i");
	auto scanner = Scanner{words};
	result = parser.parse(scanner);
	EXPECT(result.exit_code() == ExitCode::OptionRequiresArgument);
}

TEST_CASE(command_parser_reparse) {
	auto context = MockContext{};
	auto parser = CommandParser{context};
	auto arg = std::string_view{};

	auto result = parser.set_parameters({},
										{
											command("run", {positional_required(arg, "ARG")}),
										},
										CommandPolicy::Required);
	EXPECT(!result.should_early_exit());

	auto words = to_words("run first");
	auto scanner = Scanner{words};
	result = parser.parse(scanner);
	EXPECT(!result.should_early_exit());
	EXPECT(arg == "first");

	words = to_words("run second");
	scanner = Scanner{words};
	result = parser.parse(scanner);
	EXPECT(!result.should_early_exit());
	EXPECT(arg == "second");
}
} // namespace
} // namespace clap::test
