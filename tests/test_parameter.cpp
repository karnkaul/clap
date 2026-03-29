#include "clap/parameter.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <string_view>

namespace {
template <typename Type>
struct Fixture {
	Type value{};
	clap::parameter::Parse parse{clap::parameter::create_parse(value)};
};

TEST_CASE(parameter_parse_bool) {
	auto fixture = Fixture<bool>{};
	EXPECT(fixture.parse("true") && fixture.value);
	EXPECT(fixture.parse("False") && !fixture.value);
	EXPECT(fixture.parse("1") && fixture.value);
	EXPECT(fixture.parse("0") && !fixture.value);
	EXPECT(fixture.parse("-42") && fixture.value);
	EXPECT(!fixture.parse("foo"));
}

TEST_CASE(parameter_parse_number) {
	{
		auto fixture = Fixture<int>{};
		EXPECT(fixture.parse("-42") && fixture.value == -42);
		EXPECT(!fixture.parse("3.14"));
		EXPECT(!fixture.parse("24foo"));
	}
	{
		auto fixture = Fixture<float>{};
		EXPECT(fixture.parse("-42") && fixture.value == -42);
		EXPECT(fixture.parse("3.14") && std::abs(fixture.value - 3.14) < 0.0001f);
		EXPECT(!fixture.parse("24foo"));
	}
}

TEST_CASE(parameter_parse_string) {
	{
		auto fixture = Fixture<std::string_view>{};
		EXPECT(fixture.parse("foo") && fixture.value == "foo");
	}
	{
		auto fixture = Fixture<std::string>{};
		EXPECT(fixture.parse("foo") && fixture.value == "foo");
	}
}
} // namespace
