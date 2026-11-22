import clap.parser;
import clap.scanner;

#include "klib/unit_test/unit_test.hpp"

namespace {
using namespace clap;

[[nodiscard]] auto to_tokens(std::string_view const input) {
	auto const words = to_words(input);
	auto ret = std::vector<ScannerToken>{};
	ret.reserve(words.size());
	auto scanner = Scanner{words};
	auto token = ScannerToken{};
	while (scanner.scan_next(token)) {
		ret.push_back(token);
	}
	return ret;
}

class MockContext : public IContext {
  public:
	[[nodiscard]] auto get_named_parameters() const -> std::span<parameter::Named const> final { return m_named; }

	void print_help_text() final { m_help_printed = true; }
	[[nodiscard]] auto has_version() const -> bool final { return m_has_version; }
	void print_version() final { m_version_printed = true; }

	void print_error([[maybe_unused]] std::string_view message) final {}

	std::vector<parameter::Named> m_named{};

	bool m_help_printed{};
	bool m_has_version{};
	bool m_version_printed{};
	std::string m_error{};
};

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

TEST_CASE(option_parser_one_flag) {
	auto flag = bool{};
	auto fixture = Fixture{
		OptionList{
			named_flag(flag, "f,flag"),
		},
	};
	fixture.tokens = to_tokens("--flag");

	auto action = fixture.parser.parse_next(fixture.tokens.front());
	ASSERT(std::holds_alternative<OptionParser::Iterate>(action));
	EXPECT(std::get<OptionParser::Iterate>(action) == OptionParser::Iterate::Consume);
	EXPECT(flag == true);

	fixture.tokens = to_tokens("-f");
	flag = false;

	action = fixture.parser.parse_next(fixture.tokens.front());
	ASSERT(std::holds_alternative<OptionParser::Iterate>(action));
	EXPECT(std::get<OptionParser::Iterate>(action) == OptionParser::Iterate::Consume);
	EXPECT(flag == true);

	fixture.tokens = to_tokens("--foo");
	action = fixture.parser.parse_next(fixture.tokens.front());
	ASSERT(std::holds_alternative<Result>(action));
	EXPECT(std::get<Result>(action).exit_code() == ExitCode::UnrecognizedOption);
}

TEST_CASE(option_parser_multiple_named) {
	auto flag = bool{};
	auto str = std::string_view{};
	auto s32 = int{};
	auto fixture = Fixture{
		OptionList{
			named_flag(flag, "f,flag"),
			named_option(str, "s,str"),
			named_option(s32, "i"),
		},
	};

	fixture.tokens = to_tokens("-f arg --str=foo -i=42");
	EXPECT(fixture.tokens.size() == 4);

	for (auto i = 0uz; i < fixture.tokens.size(); ++i) {
		auto action = fixture.parser.parse_next(fixture.tokens.at(i));

		switch (i) {
		case 0: {
			ASSERT(std::holds_alternative<OptionParser::Iterate>(action));
			EXPECT(std::get<OptionParser::Iterate>(action) == OptionParser::Iterate::Consume);
			EXPECT(flag == true);
			break;
		}

		case 1: {
			ASSERT(std::holds_alternative<OptionParser::Iterate>(action));
			EXPECT(std::get<OptionParser::Iterate>(action) == OptionParser::Iterate::Pass);
			break;
		}

		case 2: {
			ASSERT(std::holds_alternative<OptionParser::Iterate>(action));
			EXPECT(std::get<OptionParser::Iterate>(action) == OptionParser::Iterate::Consume);
			EXPECT(str == "foo");
			break;
		}

		case 3: {
			ASSERT(std::holds_alternative<OptionParser::Iterate>(action));
			EXPECT(std::get<OptionParser::Iterate>(action) == OptionParser::Iterate::Consume);
			EXPECT(s32 == 42);
			break;
		}
		}
	}

	EXPECT(fixture.parser.finish().exit_code() == ExitCode::Success);
}

TEST_CASE(option_parser_terminate_options) {
	auto fixture = Fixture{{}};
	fixture.tokens = to_tokens("-- -foo --bar");
	EXPECT(fixture.tokens.size() == 3);
	for (auto i = 0uz; i < fixture.tokens.size(); ++i) {
		auto action = fixture.parser.parse_next(fixture.tokens.at(i));

		switch (i) {
		case 0: {
			ASSERT(std::holds_alternative<OptionParser::Iterate>(action));
			EXPECT(std::get<OptionParser::Iterate>(action) == OptionParser::Iterate::Consume);
			break;
		}

		case 1:
		case 2: {
			ASSERT(std::holds_alternative<OptionParser::Iterate>(action));
			EXPECT(std::get<OptionParser::Iterate>(action) == OptionParser::Iterate::Pass);
			break;
		}
		}
	}

	EXPECT(fixture.parser.finish().exit_code() == ExitCode::Success);
}

TEST_CASE(option_parser_missing_value) {
	auto flag = bool{};
	auto s32 = int{};
	auto fixture = Fixture{
		OptionList{
			named_flag(flag, "f,flag"),
			named_option(s32, "i"),
		},
	};

	fixture.tokens = to_tokens("-fi");
	auto action = fixture.parser.parse_next(fixture.tokens.front());
	ASSERT(std::holds_alternative<OptionParser::Iterate>(action));
	EXPECT(std::get<OptionParser::Iterate>(action) == OptionParser::Iterate::Consume);

	EXPECT(flag == true);
	EXPECT(fixture.parser.finish().exit_code() == ExitCode::OptionRequiresArgument);
}
} // namespace
