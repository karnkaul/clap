#include "clap/parameter.hpp"
#include "detail/parser.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <array>
#include <string_view>

namespace {
using namespace clap;
using namespace std::string_view_literals;

auto get_outcome(std::span<Parameter const> parameters, std::vector<std::string_view> const& args) {
	auto parse_input = detail::ParseInput{
		.parameters = parameters,
		.program = detail::ParseProgram{.name = "clap-test"},
	};
	auto parser = detail::Parser{parse_input, args};
	return parser.parse().outcome;
}

TEST_CASE(parser_flags) {
	auto a = false;
	auto b = false;
	auto const parameters = std::array{
		named_flag(a, "a,flaga"),
		named_flag(b, "b,flagb"),
	};

	auto outcome = get_outcome(parameters, {"-ab"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(a);
	EXPECT(b);

	a = b = false;
	outcome = get_outcome(parameters, {"--flaga", "-b=true"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(a);
	EXPECT(b);

	a = b = false;
	outcome = get_outcome(parameters, {"--flagb=1", "-a"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(a);
	EXPECT(b);

	a = b = false;
	outcome = get_outcome(parameters, {"--flaga", "--flagb"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(a);
	EXPECT(b);

	outcome = get_outcome(parameters, {"-ab", "--help"});
	EXPECT(outcome == detail::ParseOutcome::EarlyExit);

	auto thrown = false;
	try {
		outcome = get_outcome(parameters, {"--flagb=abc", "-a"});
	} catch (detail::error::Parse const err) {
		thrown = true;
		EXPECT(err == detail::error::Parse::InvalidArgument);
	}
	EXPECT(thrown);

	thrown = false;
	try {
		outcome = get_outcome(parameters, {"-a=foo"});
	} catch (detail::error::Parse const err) {
		thrown = true;
		EXPECT(err == detail::error::Parse::InvalidArgument);
	}
	EXPECT(thrown);
}

TEST_CASE(parser_options) {
	auto a = std::string_view{};
	auto b = int{};
	auto const parameters = std::array{
		named_option(a, "a,opta"),
		named_option(b, "b,optb"),
	};

	auto outcome = get_outcome(parameters, {"-a=x", "-b", "42"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(a == "x");
	EXPECT(b == 42);

	a = {};
	b = {};
	outcome = get_outcome(parameters, {"--opta=x", "-b=42"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(a == "x");
	EXPECT(b == 42);

	a = {};
	b = {};
	outcome = get_outcome(parameters, {"--optb=42", "-a", "x"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(a == "x");
	EXPECT(b == 42);

	a = {};
	b = {};
	auto thrown = false;
	try {
		outcome = get_outcome(parameters, {"--optb=abc", "-a=x"});
	} catch (detail::error::Parse const err) {
		thrown = true;
		EXPECT(err == detail::error::Parse::InvalidArgument);
	}
	EXPECT(thrown);

	thrown = false;
	try {
		outcome = get_outcome(parameters, {"--version"});
	} catch (detail::error::Parse const err) {
		thrown = true;
		EXPECT(err == detail::error::Parse::UnrecognizedOption);
	}
	EXPECT(thrown);

	thrown = false;
	try {
		outcome = get_outcome(parameters, {"--optb"});
	} catch (detail::error::Parse const err) {
		thrown = true;
		EXPECT(err == detail::error::Parse::OptionRequiresArgument);
	}
	EXPECT(thrown);

	thrown = false;
	try {
		outcome = get_outcome(parameters, {"-b"});
	} catch (detail::error::Parse const err) {
		thrown = true;
		EXPECT(err == detail::error::Parse::OptionRequiresArgument);
	}
	EXPECT(thrown);
}

TEST_CASE(parser_required) {
	auto a = int{};
	auto b = std::string{};
	auto const parameters = std::array{
		positional_required(a, "arga"),
		positional_required(b, "argb"),
	};

	auto outcome = get_outcome(parameters, {"42", "y"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(a == 42);
	EXPECT(b == "y");

	a = {};
	b.clear();
	auto thrown = false;
	try {
		outcome = get_outcome(parameters, {"42"});
	} catch (detail::error::Parse const error) {
		thrown = true;
		EXPECT(error == detail::error::Parse::MissingRequiredArgument);
	}
	EXPECT(thrown);

	a = {};
	b.clear();
	thrown = false;
	try {
		outcome = get_outcome(parameters, {"42", "y", "foo"});
	} catch (detail::error::Parse const error) {
		thrown = true;
		EXPECT(error == detail::error::Parse::UnknownArgument);
	}
	EXPECT(thrown);

	a = {};
	b.clear();
	thrown = false;
	try {
		outcome = get_outcome(parameters, {"foo", "y"});
	} catch (detail::error::Parse const error) {
		thrown = true;
		EXPECT(error == detail::error::Parse::InvalidArgument);
	}
	EXPECT(thrown);
}

TEST_CASE(parser_optional) {
	auto a = std::string_view{};
	auto b = int{};
	auto const parameters = std::array{
		positional_required(a, "arga"),
		positional_optional(b, "argb"),
	};

	auto outcome = get_outcome(parameters, {"x", "42"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(a == "x");
	EXPECT(b == 42);

	a = {};
	b = -5;
	outcome = get_outcome(parameters, {"x"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(a == "x");
	EXPECT(b == -5);

	auto thrown = false;
	try {
		outcome = get_outcome(parameters, {});
	} catch (detail::error::Parse const error) {
		thrown = true;
		EXPECT(error == detail::error::Parse::MissingRequiredArgument);
	}
	EXPECT(thrown);

	thrown = false;
	try {
		outcome = get_outcome(parameters, {"x", "42", "foo"});
	} catch (detail::error::Parse const error) {
		thrown = true;
		EXPECT(error == detail::error::Parse::UnknownArgument);
	}
	EXPECT(thrown);
}

TEST_CASE(parser_list) {
	auto flag = bool{};
	auto list = std::vector<std::string_view>{};
	auto const parameters = std::array{
		named_flag(flag, "f,flag"),
		positional_list(list, "list"),
	};

	auto outcome = get_outcome(parameters, {"-f", "zero", "one", "two"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(flag);
	ASSERT(list.size() == 3);
	EXPECT(list[0] == "zero");
	EXPECT(list[1] == "one");
	EXPECT(list[2] == "two");

	flag = {};
	list.clear();
	outcome = get_outcome(parameters, {"--flag"});
	EXPECT(outcome == detail::ParseOutcome::Continue);
	EXPECT(flag);
	ASSERT(list.empty());
}

TEST_CASE(parser_parameter_errors) {
	auto flag = bool{};
	auto i = int{};
	auto str = std::string_view{};
	auto list = std::vector<std::string_view>{};

	auto thrown = false;
	try {
		auto const parameters = std::array{
			named_flag(flag, "f0,flag0"),
			positional_list(list, "list0"),
			positional_required(str, "str0"),
		};
		get_outcome(parameters, {});
	} catch (detail::error::Parameter const err) {
		thrown = true;
		EXPECT(err == detail::error::Parameter::ExtraneousPositional);
	}
	EXPECT(thrown);

	thrown = false;
	try {
		auto const parameters = std::array{
			positional_optional(str, "str1"),
			positional_required(i, "int1"),
		};
		get_outcome(parameters, {});
	} catch (detail::error::Parameter const err) {
		thrown = true;
		EXPECT(err == detail::error::Parameter::ExtraneousPositional);
	}
	EXPECT(thrown);
}

TEST_CASE(parser_unexpected_tokens) {
	auto flag = bool{};

	auto thrown = false;
	try {
		auto const parameters = std::array{
			named_flag(flag, "f,flag"),
		};
		get_outcome(parameters, {"-f=true=false"});
	} catch (detail::error::Parse const err) {
		thrown = true;
		EXPECT(err == detail::error::Parse::UnexpectedToken);
	}
	EXPECT(thrown);
}
} // namespace
