#include <clap/arg.hpp>
#include <ktest/ktest.hpp>
#include <ranges>

namespace {
using namespace cliq;

TEST(arg_flag) {
	bool flag{};
	auto const arg = Arg{flag, "f,flag"};
	auto const& param = arg.get_param();
	ASSERT(std::holds_alternative<ParamOption>(param));
	auto const& option = std::get<ParamOption>(param);
	EXPECT(option.is_flag);
	EXPECT(option.letter == 'f');
	EXPECT(option.word == "flag");
	EXPECT(flag == false);
	EXPECT(option.assign({}));
	EXPECT(flag == true);
}

TEST(arg_option_number) {
	int x{};
	auto arg = Arg{x, "x"};
	auto param = arg.get_param();
	ASSERT(std::holds_alternative<ParamOption>(param));
	auto option = std::get<ParamOption>(param);
	EXPECT(!option.is_flag);
	EXPECT(option.letter == 'x');
	EXPECT(option.word.empty());
	EXPECT(x == 0);
	EXPECT(option.assign("42"));
	EXPECT(x == 42);
	EXPECT(!option.assign("abc"));

	float foo{};
	arg = Arg{foo, "foo"};
	param = arg.get_param();
	ASSERT(std::holds_alternative<ParamOption>(param));
	option = std::get<ParamOption>(param);
	EXPECT(!option.is_flag);
	EXPECT(option.letter == '\0');
	EXPECT(option.word == "foo");
	EXPECT(foo == 0.0f);
	EXPECT(option.assign("3.14"));
	EXPECT(std::abs(foo - 3.14) < 0.001f);
	EXPECT(!option.assign("abc"));
}

TEST(arg_positional_number) {
	int x{};
	auto arg = Arg{x, ArgType::Required, "x"};
	auto param = arg.get_param();
	ASSERT(std::holds_alternative<ParamPositional>(param));
	auto positional = std::get<ParamPositional>(param);
	EXPECT(positional.arg_type == ArgType::Required);
	EXPECT(positional.name == "x");
	EXPECT(positional.assign("42") && x == 42);
	EXPECT(!positional.assign("abc"));

	float foo{};
	arg = Arg{foo, ArgType::Optional, "foo"};
	param = arg.get_param();
	ASSERT(std::holds_alternative<ParamPositional>(param));
	positional = std::get<ParamPositional>(param);
	EXPECT(positional.arg_type == ArgType::Optional);
	EXPECT(positional.name == "foo");
	EXPECT(positional.assign("3.14") && std::abs(foo - 3.14) < 0.001f);
	EXPECT(!positional.assign("abc"));
}

TEST(arg_string) {
	std::string_view foo{};
	auto arg = Arg{foo, "foo"};
	auto param = arg.get_param();
	ASSERT(std::holds_alternative<ParamOption>(param));
	auto const& option = std::get<ParamOption>(param);
	EXPECT(!option.is_flag);
	EXPECT(option.letter == '\0');
	EXPECT(option.word == "foo");
	EXPECT(option.assign("bar"));
	EXPECT(foo == "bar");

	foo = {};
	arg = Arg{foo, ArgType::Optional, "foo"};
	param = arg.get_param();
	ASSERT(std::holds_alternative<ParamPositional>(param));
	auto const& positional = std::get<ParamPositional>(param);
	EXPECT(positional.arg_type == ArgType::Optional);
	EXPECT(positional.name == "foo");
	EXPECT(positional.assign("bar") && foo == "bar");
}

TEST(arg_command) {
	struct CmdParams {
		bool verbose{};
		int number{};
	};
	auto params = CmdParams{};
	auto const args = std::array{
		Arg{params.verbose, "v,verbose"},
		Arg{params.number, ArgType::Required, "number"},
	};
	auto const cmd_arg = Arg{args, "cmd"};
	auto param = cmd_arg.get_param();
	ASSERT(std::holds_alternative<ParamCommand>(param));
	auto const& cmd = std::get<ParamCommand>(param);
	EXPECT(cmd.name == "cmd");
	for (auto const& [a, b] : std::ranges::zip_view(cmd.args, args)) { EXPECT(a.get_param().index() == b.get_param().index()); }
}
} // namespace
