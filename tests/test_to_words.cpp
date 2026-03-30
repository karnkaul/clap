#include "clap/parser.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <string_view>

namespace {
using namespace clap;

TEST_CASE(to_words) {
	auto line = std::string_view{" foo   bar "};
	auto words = to_words(line);
	ASSERT(words.size() == 2);
	EXPECT(words[0] == "foo");
	EXPECT(words[1] == "bar");

	line = R"("foo bar"		fubar)";
	words = to_words(line);
	ASSERT(words.size() == 2);
	EXPECT(words[0] == "foo bar");
	EXPECT(words[1] == "fubar");

	line = R"(fubar"foo bar)";
	words = to_words(line);
	ASSERT(words.size() == 2);
	EXPECT(words[0] == "fubar");
	EXPECT(words[1] == "foo bar");
}
} // namespace
