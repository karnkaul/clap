import clap.detail;
import clap.scanner;

#include "common.hpp"
#include "klib/unit_test/unit_test.hpp"

namespace clap::test {
namespace {
using detail::OptionParser;
using Iterate = OptionParser::Iterate;
using detail::ParseException;

class Fixture {
  public:
	explicit Fixture(OptionList options) : options(std::move(options)), parser(context, this->options) {}

	MockContext context{};
	OptionList options;
	OptionParser parser;

	std::vector<ScannerToken> tokens{};
};

TEST_CASE(option_parser_builtins) {
	auto fixture = Fixture{{}};
	fixture.tokens = to_tokens("--help");
	auto action = fixture.parser.parse_next(fixture.tokens.front());
	EXPECT(action == Iterate::EarlyExit);
	EXPECT(fixture.context.m_help_printed == true);

	fixture.tokens = to_tokens("--version");
	fixture.context.m_has_version = true;
	action = fixture.parser.parse_next(fixture.tokens.front());
	EXPECT(action == Iterate::EarlyExit);
	EXPECT(fixture.context.m_version_printed == true);

	fixture.context.m_version_printed = false;
	fixture.context.m_has_version = false;
	auto exit_code = ExitCode::Success;
	try {
		action = fixture.parser.parse_next(fixture.tokens.front());
	} catch (ParseException const& e) {
		exit_code = e.get_exit_code();
	}
	EXPECT(exit_code == ExitCode::UnrecognizedOption);
	EXPECT(fixture.context.m_version_printed == false);
}

TEST_CASE(option_parser_one_flag) {
	auto flag = bool{};
	auto fixture = Fixture{
		OptionList{
			named_flag(flag, "f,flag"),
		},
	};
	fixture.tokens = to_tokens("--flag");

	auto action = fixture.parser.parse_next(fixture.tokens.front());
	EXPECT(action == Iterate::Consume);
	EXPECT(flag == true);

	fixture.tokens = to_tokens("-f");
	flag = false;

	action = fixture.parser.parse_next(fixture.tokens.front());
	EXPECT(action == Iterate::Consume);
	EXPECT(flag == true);

	fixture.tokens = to_tokens("--foo");
	auto exit_code = ExitCode::Success;
	try {
		action = fixture.parser.parse_next(fixture.tokens.front());
	} catch (ParseException const& e) {
		exit_code = e.get_exit_code();
	}
	EXPECT(exit_code == ExitCode::UnrecognizedOption);
}

TEST_CASE(option_parser_multiple_named) {
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
			EXPECT(action == Iterate::Consume);
			EXPECT(flag == true);
			break;
		}

		case 1: {
			EXPECT(action == Iterate::Pass);
			break;
		}

		case 2: {
			EXPECT(action == Iterate::Consume);
			EXPECT(str == "foo");
			break;
		}

		case 3: {
			EXPECT(action == Iterate::Consume);
			EXPECT(i32 == 42);
			break;
		}
		}
	}

	auto thrown = false;
	try {
		fixture.parser.finish();
	} catch (ParseException const&) {
		thrown = true;
	}
	EXPECT(!thrown);
}

TEST_CASE(option_parser_terminate_options) {
	auto fixture = Fixture{{}};
	fixture.tokens = to_tokens("-- -foo --bar");
	EXPECT(fixture.tokens.size() == 3);
	for (auto i = 0uz; i < fixture.tokens.size(); ++i) {
		auto action = fixture.parser.parse_next(fixture.tokens.at(i));

		switch (i) {
		case 0: {
			EXPECT(action == Iterate::Consume);
			break;
		}

		case 1:
		case 2: {
			EXPECT(action == Iterate::Pass);
			break;
		}
		}
	}

	auto thrown = false;
	try {
		fixture.parser.finish();
	} catch (ParseException const&) {
		thrown = true;
	}
	EXPECT(!thrown);
}

TEST_CASE(option_parser_missing_value) {
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
	EXPECT(action == Iterate::Consume);
	EXPECT(flag == true);

	auto exit_code = ExitCode::Success;
	try {
		fixture.parser.finish();
	} catch (ParseException const& e) {
		exit_code = e.get_exit_code();
	}
	EXPECT(exit_code == ExitCode::OptionRequiresArgument);
}
} // namespace
} // namespace clap::test
