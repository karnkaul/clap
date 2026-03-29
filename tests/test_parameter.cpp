#include "clap/parameter.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <string_view>

namespace {
template <typename Type>
struct Fixture {
	Type value{};
	bool was_set{};
	clap::parameter::Parse parse{clap::parameter::create_parse(value, &was_set)};
};

TEST_CASE(parameter_parse_bool) {
	auto fixture = Fixture<bool>{};
	EXPECT(!fixture.was_set);
	EXPECT(fixture.parse("true") && fixture.value && fixture.was_set);
	EXPECT(fixture.parse("False") && !fixture.value);
	EXPECT(fixture.parse("1") && fixture.value);
	EXPECT(fixture.parse("0") && !fixture.value);
	EXPECT(fixture.parse("-42") && fixture.value);
	EXPECT(!fixture.parse("foo"));
}

TEST_CASE(parameter_parse_number) {
	{
		auto fixture = Fixture<int>{};
		EXPECT(!fixture.was_set);
		EXPECT(fixture.parse("-42") && fixture.value == -42 && fixture.was_set);
		EXPECT(!fixture.parse("3.14"));
		EXPECT(!fixture.parse("24foo"));
	}
	{
		auto fixture = Fixture<float>{};
		EXPECT(!fixture.was_set);
		EXPECT(fixture.parse("-42"));
		EXPECT(fixture.parse("3.14") && std::abs(fixture.value - 3.14) < 0.0001f);
		EXPECT(!fixture.parse("24foo"));
	}
}

TEST_CASE(parameter_parse_string) {
	{
		auto fixture = Fixture<std::string_view>{};
		EXPECT(!fixture.was_set);
		EXPECT(fixture.parse("foo") && fixture.value == "foo" && fixture.was_set);
	}
	{
		auto fixture = Fixture<std::string>{};
		EXPECT(!fixture.was_set);
		EXPECT(fixture.parse("foo") && fixture.value == "foo" && fixture.was_set);
	}
}
} // namespace
