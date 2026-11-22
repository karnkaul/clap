import clap.parser;

#include "common.hpp"
#include "klib/unit_test/unit_test.hpp"

namespace clap::test {
namespace {
class Fixture {
  public:
	[[nodiscard]] auto parse(ParameterList parameters, std::string_view const expr) {
		// NOLINTNEXTLINE(readability-trailing-comma)
		auto ret = environment.build({}, std::move(parameters));
		if (ret != ExitCode::Success) {
			return Result{ret, Outcome::EarlyExit, {}};
		}

		auto parser = ParameterParser{context, environment};
		auto const args = to_words(expr);
		auto scanner = Scanner{args};
		return parser.parse(scanner);
	}

	MockContext context{};
	ParameterEnvironment environment;

	std::vector<ScannerToken> tokens{};
};

TEST_CASE(parameter_parser_one_arg) {
	auto fixture = Fixture{};
	auto flag = bool{};
	auto i32 = int{};
	auto arg = std::string_view{};
	auto result = fixture.parse(
		{
			named_flag(flag, "f,flag"),
			named_option(i32, "i"),
			positional_required(arg, "ARG"),
		},
		"-f foo -i=42");
	EXPECT(!result.should_early_exit());
	EXPECT(flag == true);
	EXPECT(i32 == 42);
	EXPECT(arg == "foo");
}

TEST_CASE(parameter_parser_optional_arg) {
	auto fixture = Fixture{};
	auto flag = bool{};
	auto i32 = int{};
	auto arg = std::string_view{};
	auto result = fixture.parse(
		{
			named_flag(flag, "f,flag"),
			named_option(i32, "i"),
			positional_optional(arg, "ARG"),
		},
		"-f -i=42");

	EXPECT(!result.should_early_exit());
	EXPECT(flag == true);
	EXPECT(i32 == 42);
	EXPECT(arg.empty());
}

TEST_CASE(parameter_parser_list_arg) {
	auto fixture = Fixture{};
	auto words = std::vector<std::string_view>{};
	auto result = fixture.parse(
		{
			positional_list(words, "WORDS"),
		},
		"foo bar fubar");
	EXPECT(!result.should_early_exit());
	ASSERT(words.size() == 3);
	EXPECT(words[0] == "foo");
	EXPECT(words[1] == "bar");
	EXPECT(words[2] == "fubar");
}

TEST_CASE(parameter_parser_multiple_args) {
	auto fixture = Fixture{};

	auto pre = std::string_view{};
	auto list = std::vector<std::string_view>{};
	auto post = std::string_view{};

	auto result = fixture.parse(
		{
			positional_required(pre, "PRE"),
			positional_list(list, "LIST"),
			positional_required(post, "POST"),
		},
		"pre one two three post");
	EXPECT(!result.should_early_exit());
	EXPECT(pre == "pre");
	ASSERT(list.size() == 3);
	EXPECT(list[0] == "one");
	EXPECT(list[1] == "two");
	EXPECT(list[2] == "three");
	EXPECT(post == "post");
}

TEST_CASE(parameter_parser_multiple_options_args) {
	auto fixture = Fixture{};

	auto flag = bool{};
	auto str = std::string_view{};
	auto pre = std::string_view{};
	auto list = std::vector<std::string_view>{};
	auto post = std::string_view{};

	auto result = fixture.parse(
		{
			named_flag(flag, "f,flag"),
			named_option(str, "str"),
			positional_required(pre, "PRE"),
			positional_list(list, "LIST"),
			positional_required(post, "POST"),
		},
		"pre -f one two --str=\"quoted string\" three post");
	EXPECT(!result.should_early_exit());
	EXPECT(flag == true);
	EXPECT(str == "quoted string");
	EXPECT(pre == "pre");
	ASSERT(list.size() == 3);
	EXPECT(list[0] == "one");
	EXPECT(list[1] == "two");
	EXPECT(list[2] == "three");
	EXPECT(post == "post");
}

TEST_CASE(parameter_parser_missing_option_value) {
	auto fixture = Fixture{};
	auto i32 = int{};
	auto result = fixture.parse(
		{
			named_option(i32, "i"),
		},
		"-i");
	EXPECT(result.exit_code() == ExitCode::OptionRequiresArgument);
}

TEST_CASE(parameter_parser_invalid_positional_stops_remaining_positionals) {
	auto fixture = Fixture{};
	auto number = int{};
	auto post = std::string_view{};
	auto result = fixture.parse(
		{
			positional_required(number, "NUMBER"),
			positional_required(post, "POST"),
		},
		"not-a-number should-not-be-assigned");
	EXPECT(result.exit_code() == ExitCode::InvalidArgument);
	EXPECT(post.empty());
}

TEST_CASE(parameter_parser_reparse_clears_previous_arguments) {
	auto fixture = Fixture{};
	auto arg = std::string_view{};

	auto first_result = fixture.parse(
		{
			positional_required(arg, "ARG"),
		},
		"first");
	EXPECT(!first_result.should_early_exit());
	EXPECT(arg == "first");

	auto second_result = fixture.parse(
		{
			positional_required(arg, "ARG"),
		},
		"second");
	EXPECT(!second_result.should_early_exit());
	EXPECT(arg == "second");
}

TEST_CASE(parameter_parser_unknown_arg) {
	auto fixture = Fixture{};
	auto result = fixture.parse({}, "foo");
	EXPECT(result.exit_code() == ExitCode::UnknownArgument);
}

TEST_CASE(parameter_parser_urecognized_option) {
	auto fixture = Fixture{};
	auto const result = fixture.parse({}, "--foo");
	EXPECT(result.exit_code() == ExitCode::UnrecognizedOption);
}
} // namespace
} // namespace clap::test
