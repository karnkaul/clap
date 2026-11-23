import clap.detail;
import clap.build_version;

#include "klib/unit_test/unit_test.hpp"

namespace clap::test {
namespace {
using detail::ProgramContext;

constexpr auto program_v = Program{
	.name = "clap",
	.version = build_version_v,
	.description = "program context tests",
};

class Context : public ProgramContext {
  public:
	explicit Context(Program const& program,
					 std::variant<ParameterList, OptionList> program_parameters,
					 CommandList commands,
					 CommandPolicy command_policy,
					 IPrinter* custom_printer)
		: ProgramContext(program, std::move(program_parameters), std::move(commands), command_policy, custom_printer) {}

	[[nodiscard]] static auto create_parameters(ParameterList parameters = {}, Program const& program = program_v)
		-> Context {
		return Context{program, std::move(parameters), {}, {}, {}};
	}

	[[nodiscard]] static auto create_commands(CommandList commands,
											  CommandPolicy policy = CommandPolicy::Required,
											  Program const& program = program_v,
											  OptionList options = {}) -> Context {
		return Context{program, std::move(options), std::move(commands), policy, {}};
	}

	void print_help_text() final {
		help_printed = true;
		ProgramContext::print_help_text();
	}

	void print_version() override {
		version_printed = has_version();
		ProgramContext::print_version();
	}

	bool help_printed{};
	bool version_printed{};
};

TEST_CASE(program_context_version) {
	auto context = Context::create_parameters();

	context.print_version();
	EXPECT(context.version_printed);

	context = Context::create_commands({
		command("cmd", {}),
	});
	context.set_selected_command(&context.get_environment().commands.front());
	context.print_version();
	EXPECT(!context.version_printed);
}

TEST_CASE(program_context_help_parameters) {
	auto context = Context::create_parameters();
	context.print_help_text();
	EXPECT(context.help_printed);

	auto flag = bool{};
	auto i32 = int{};
	auto required = std::string_view{};
	auto optional = std::string_view{};
	context = Context::create_parameters({
		named_flag(flag, "f,flag", "test flag"),
		named_option(i32, "i", "test i32"),
		positional_required(required, "REQ", "test required positional"),
		positional_optional(optional, "OPT", "test optional positional"),
	});

	context.print_help_text();
	EXPECT(context.help_printed);
}

TEST_CASE(program_context_help_commands) {
	auto flag = bool{};
	auto i32 = int{};
	auto required = std::string_view{};
	auto optional = std::string_view{};

	auto context = Context::create_commands({
		command("cmd1",
				{
					named_flag(flag, "f,flag", "test flag"),
					named_option(i32, "i", "test i32"),
					positional_required(required, "REQ", "test required positional"),
					positional_optional(optional, "OPT", "test optional positional"),
				},
				"test command 1"),
		command("cmd2", {}, "test command 2"),
	});
	context.print_help_text();
	context.set_selected_command(&context.get_environment().commands.front());
	context.print_help_text();
	context.set_selected_command(&context.get_environment().commands[1]);
	context.print_help_text();
	EXPECT(context.help_printed);
}
} // namespace
} // namespace clap::test
