#include "clap/exception.hpp"
#include "clap/parameter.hpp"
#include "clap/spec.hpp"
#include "detail/parser_impl.hpp"
#include "detail/types.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <string_view>

namespace {
using namespace clap;
using namespace std::string_view_literals;

auto get_outcome(ParameterList parameters, std::vector<std::string_view> const& args) {
	auto spec = spec::Parameters{
		.parameters = std::move(parameters),
		.program = Program{.name = "clap-test"},
	};
	auto parse_context = detail::Context{std::move(spec)};
	auto parser = detail::ParserImpl{parse_context, args};
	return parser.parse().outcome;
}

TEST_CASE(parser_flags) {
	auto a = false;
	auto b = false;
	auto const parameters = ParameterList{
		named_flag(a, "a,flaga"),
		named_flag(b, "b,flagb"),
	};

	auto outcome = get_outcome(parameters, {"-ab"});
	EXPECT(outcome == Outcome::Continue);
	EXPECT(a);
	EXPECT(b);

	a = b = false;
	outcome = get_outcome(parameters, {"--flaga", "-b=true"});
	EXPECT(outcome == Outcome::Continue);
	EXPECT(a);
	EXPECT(b);

	a = b = false;
	outcome = get_outcome(parameters, {"--flagb=1", "-a"});
	EXPECT(outcome == Outcome::Continue);
	EXPECT(a);
	EXPECT(b);

	a = b = false;
	outcome = get_outcome(parameters, {"--flaga", "--flagb"});
	EXPECT(outcome == Outcome::Continue);
	EXPECT(a);
	EXPECT(b);

	outcome = get_outcome(parameters, {"-ab", "--help"});
	EXPECT(outcome == Outcome::EarlyExit);

	auto thrown = false;
	try {
		outcome = get_outcome(parameters, {"--flagb=abc", "-a"});
	} catch (detail::Error const err) {
		thrown = true;
		EXPECT(err == detail::Error::InvalidArgument);
	}
	EXPECT(thrown);

	thrown = false;
	try {
		outcome = get_outcome(parameters, {"-a=foo"});
	} catch (detail::Error const err) {
		thrown = true;
		EXPECT(err == detail::Error::InvalidArgument);
	}
	EXPECT(thrown);
}

TEST_CASE(parser_options) {
	auto a = std::string_view{};
	auto b = int{};
	auto const parameters = std::vector<Parameter>{
		named_option(a, "a,opta"),
		named_option(b, "b,optb"),
	};

	auto outcome = get_outcome(parameters, {"-a=x", "-b", "42"});
	EXPECT(outcome == Outcome::Continue);
	EXPECT(a == "x");
	EXPECT(b == 42);

	a = {};
	b = {};
	outcome = get_outcome(parameters, {"--opta=x", "-b=42"});
	EXPECT(outcome == Outcome::Continue);
	EXPECT(a == "x");
	EXPECT(b == 42);

	a = {};
	b = {};
	outcome = get_outcome(parameters, {"--optb=42", "-a", "x"});
	EXPECT(outcome == Outcome::Continue);
	EXPECT(a == "x");
	EXPECT(b == 42);

	a = {};
	b = {};
	auto thrown = false;
	try {
		outcome = get_outcome(parameters, {"--optb=abc", "-a=x"});
	} catch (detail::Error const err) {
		thrown = true;
		EXPECT(err == detail::Error::InvalidArgument);
	}
	EXPECT(thrown);

	thrown = false;
	try {
		outcome = get_outcome(parameters, {"--version"});
	} catch (detail::Error const err) {
		thrown = true;
		EXPECT(err == detail::Error::UnrecognizedOption);
	}
	EXPECT(thrown);

	thrown = false;
	try {
		outcome = get_outcome(parameters, {"--optb"});
	} catch (detail::Error const err) {
		thrown = true;
		EXPECT(err == detail::Error::OptionRequiresArgument);
	}
	EXPECT(thrown);

	thrown = false;
	try {
		outcome = get_outcome(parameters, {"-b"});
	} catch (detail::Error const err) {
		thrown = true;
		EXPECT(err == detail::Error::OptionRequiresArgument);
	}
	EXPECT(thrown);
}

TEST_CASE(parser_required) {
	auto a = int{};
	auto b = std::string{};
	auto const parameters = std::vector<Parameter>{
		positional_required(a, "arga"),
		positional_required(b, "argb"),
	};

	auto outcome = get_outcome(parameters, {"42", "y"});
	EXPECT(outcome == Outcome::Continue);
	EXPECT(a == 42);
	EXPECT(b == "y");

	a = {};
	b.clear();
	auto thrown = false;
	try {
		outcome = get_outcome(parameters, {"42"});
	} catch (detail::Error const error) {
		thrown = true;
		EXPECT(error == detail::Error::MissingRequiredArgument);
	}
	EXPECT(thrown);

	a = {};
	b.clear();
	thrown = false;
	try {
		outcome = get_outcome(parameters, {"42", "y", "foo"});
	} catch (detail::Error const error) {
		thrown = true;
		EXPECT(error == detail::Error::UnknownArgument);
	}
	EXPECT(thrown);

	a = {};
	b.clear();
	thrown = false;
	try {
		outcome = get_outcome(parameters, {"foo", "y"});
	} catch (detail::Error const error) {
		thrown = true;
		EXPECT(error == detail::Error::InvalidArgument);
	}
	EXPECT(thrown);
}

TEST_CASE(parser_optional) {
	auto a = std::string_view{};
	auto b = int{};
	auto const parameters = std::vector<Parameter>{
		positional_required(a, "arga"),
		positional_optional(b, "argb"),
	};

	auto outcome = get_outcome(parameters, {"x", "42"});
	EXPECT(outcome == Outcome::Continue);
	EXPECT(a == "x");
	EXPECT(b == 42);

	a = {};
	b = -5;
	outcome = get_outcome(parameters, {"x"});
	EXPECT(outcome == Outcome::Continue);
	EXPECT(a == "x");
	EXPECT(b == -5);

	auto thrown = false;
	try {
		outcome = get_outcome(parameters, {});
	} catch (detail::Error const error) {
		thrown = true;
		EXPECT(error == detail::Error::MissingRequiredArgument);
	}
	EXPECT(thrown);

	thrown = false;
	try {
		outcome = get_outcome(parameters, {"x", "42", "foo"});
	} catch (detail::Error const error) {
		thrown = true;
		EXPECT(error == detail::Error::UnknownArgument);
	}
	EXPECT(thrown);
}

TEST_CASE(parser_list) {
	auto flag = bool{};
	auto list = std::vector<std::string_view>{};
	auto const parameters = std::vector<Parameter>{
		named_flag(flag, "f,flag"),
		positional_list(list, "list"),
	};

	auto outcome = get_outcome(parameters, {"-f", "zero", "one", "two"});
	EXPECT(outcome == Outcome::Continue);
	EXPECT(flag);
	ASSERT(list.size() == 3);
	EXPECT(list[0] == "zero");
	EXPECT(list[1] == "one");
	EXPECT(list[2] == "two");

	flag = {};
	list.clear();
	outcome = get_outcome(parameters, {"--flag"});
	EXPECT(outcome == Outcome::Continue);
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
		auto const parameters = std::vector<Parameter>{
			named_flag(flag, "f0,flag0"),
			positional_list(list, "list0"),
			positional_required(str, "str0"),
		};
		get_outcome(parameters, {});
	} catch (InvalidParameterException const& /*err*/) { thrown = true; }
	EXPECT(thrown);

	thrown = false;
	try {
		auto const parameters = std::vector<Parameter>{
			positional_optional(str, "str1"),
			positional_required(i, "int1"),
		};
		get_outcome(parameters, {});
	} catch (InvalidParameterException const& /*err*/) { thrown = true; }
	EXPECT(thrown);
}

TEST_CASE(parser_unexpected_tokens) {
	auto flag = bool{};

	auto thrown = false;
	try {
		auto const parameters = std::vector<Parameter>{
			named_flag(flag, "f,flag"),
		};
		get_outcome(parameters, {"-f=true=false"});
	} catch (detail::Error const err) {
		thrown = true;
		EXPECT(err == detail::Error::UnexpectedToken);
	}
	EXPECT(thrown);
}
} // namespace
