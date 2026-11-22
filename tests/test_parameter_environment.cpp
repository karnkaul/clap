import clap.detail;

#include "klib/unit_test/unit_test.hpp"

namespace clap::test {
namespace {
using detail::ParameterEnvironment;

TEST_CASE(parameter_environment_invalid_positionals) {
	auto number = int{};
	auto post = std::string_view{};
	auto thrown = false;
	try {
		auto result = ParameterEnvironment{
			{},
			{
				positional_optional(number, "NUMBER"),
				positional_required(post, "POST"),
			},
		};
	} catch (InvalidParameterException const&) {
		thrown = true;
	}
	EXPECT(thrown == true);

	auto list = std::vector<std::string_view>{};
	thrown = false;
	try {
		auto result = ParameterEnvironment{
			{},
			{
				positional_list(list, "LIST1"),
				positional_list(list, "LIST2"),
			},
		};
	} catch (InvalidParameterException const&) {
		thrown = true;
	}
	EXPECT(thrown == true);
}
} // namespace
} // namespace clap::test
