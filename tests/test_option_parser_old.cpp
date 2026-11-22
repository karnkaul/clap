import clap.detail;
import clap.scanner;

#include "common.hpp"
#include "klib/unit_test/unit_test.hpp"

namespace clap::test {
namespace {
using detail::OptionParserOld;
using Iterate = OptionParserOld::Iterate;

class Fixture {
  public:
	explicit Fixture(OptionList options) : options(std::move(options)), parser(context, this->options) {}

	MockContext context{};
	OptionList options;
	OptionParserOld parser;

	std::vector<ScannerToken> tokens{};
};

TEST_CASE(option_parser_old_builtins) {
	auto fixture = Fixture{{}};
	fixture.tokens = to_tokens("--help");
	auto action = fixture.parser.parse_next(fixture.tokens.front());
	ASSERT(std::holds_alternative<Result>(action));
	EXPECT(std::get<Result>(action).exit_code() == ExitCode::Success);
	EXPECT(fixture.context.m_help_printed == true);

	fixture.tokens = to_tokens("--version");
	fixture.context.m_has_version = true;
	action = fixture.parser.parse_next(fixture.tokens.front());
	ASSERT(std::holds_alternative<Result>(action));
	EXPECT(std::get<Result>(action).exit_code() == ExitCode::Success);
	EXPECT(fixture.context.m_version_printed == true);

	fixture.context.m_version_printed = false;
	fixture.context.m_has_version = false;
	action = fixture.parser.parse_next(fixture.tokens.front());
	ASSERT(std::holds_alternative<Result>(action));
	EXPECT(std::get<Result>(action).exit_code() == ExitCode::UnrecognizedOption);
	EXPECT(fixture.context.m_version_printed == false);
}

TEST_CASE(option_parser_old_one_flag) {
	auto flag = bool{};
	auto fixture = Fixture{
		OptionList{
			named_flag(flag, "f,flag"),
		},
	};
	fixture.tokens = to_tokens("--flag");

	auto action = fixture.parser.parse_next(fixture.tokens.front());
	ASSERT(std::holds_alternative<Iterate>(action));
	EXPECT(std::get<Iterate>(action) == Iterate::Consume);
	EXPECT(flag == true);

	fixture.tokens = to_tokens("-f");
	flag = false;

	action = fixture.parser.parse_next(fixture.tokens.front());
	ASSERT(std::holds_alternative<Iterate>(action));
	EXPECT(std::get<Iterate>(action) == Iterate::Consume);
	EXPECT(flag == true);

	fixture.tokens = to_tokens("--foo");
	action = fixture.parser.parse_next(fixture.tokens.front());
	ASSERT(std::holds_alternative<Result>(action));
	EXPECT(std::get<Result>(action).exit_code() == ExitCode::UnrecognizedOption);
}

TEST_CASE(option_parser_old_multiple_named) {
	auto flag = bool{};
	auto str = std::string_view{};
	auto i32 = int{};
	auto fixture = Fixture{
		OptionList{
			named_flag(flag, "f,flag"),
			named_option(str, "s,str"),
			named_option(i32, "i"),
		},
	};

	fixture.tokens = to_tokens("-f arg --str=foo -i=42");
	EXPECT(fixture.tokens.size() == 4);

	for (auto i = 0uz; i < fixture.tokens.size(); ++i) {
		auto action = fixture.parser.parse_next(fixture.tokens.at(i));

		switch (i) {
		case 0: {
			ASSERT(std::holds_alternative<Iterate>(action));
			EXPECT(std::get<Iterate>(action) == Iterate::Consume);
			EXPECT(flag == true);
			break;
		}

		case 1: {
			ASSERT(std::holds_alternative<Iterate>(action));
			EXPECT(std::get<Iterate>(action) == Iterate::Pass);
			break;
		}

		case 2: {
			ASSERT(std::holds_alternative<Iterate>(action));
			EXPECT(std::get<Iterate>(action) == Iterate::Consume);
			EXPECT(str == "foo");
			break;
		}

		case 3: {
			ASSERT(std::holds_alternative<Iterate>(action));
			EXPECT(std::get<Iterate>(action) == Iterate::Consume);
			EXPECT(i32 == 42);
			break;
		}
		}
	}

	EXPECT(fixture.parser.finish().exit_code() == ExitCode::Success);
}

TEST_CASE(option_parser_old_terminate_options) {
	auto fixture = Fixture{{}};
	fixture.tokens = to_tokens("-- -foo --bar");
	EXPECT(fixture.tokens.size() == 3);
	for (auto i = 0uz; i < fixture.tokens.size(); ++i) {
		auto action = fixture.parser.parse_next(fixture.tokens.at(i));

		switch (i) {
		case 0: {
			ASSERT(std::holds_alternative<Iterate>(action));
			EXPECT(std::get<Iterate>(action) == Iterate::Consume);
			break;
		}

		case 1:
		case 2: {
			ASSERT(std::holds_alternative<Iterate>(action));
			EXPECT(std::get<Iterate>(action) == Iterate::Pass);
			break;
		}
		}
	}

	EXPECT(fixture.parser.finish().exit_code() == ExitCode::Success);
}

TEST_CASE(option_parser_old_missing_value) {
	auto flag = bool{};
	auto i32 = int{};
	auto fixture = Fixture{
		OptionList{
			named_flag(flag, "f,flag"),
			named_option(i32, "i"),
		},
	};

	fixture.tokens = to_tokens("-fi");
	auto action = fixture.parser.parse_next(fixture.tokens.front());
	ASSERT(std::holds_alternative<Iterate>(action));
	EXPECT(std::get<Iterate>(action) == Iterate::Consume);

	EXPECT(flag == true);
	EXPECT(fixture.parser.finish().exit_code() == ExitCode::OptionRequiresArgument);
}
} // namespace
} // namespace clap::test
