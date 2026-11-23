module clap.parser;

namespace clap {
Parser::Parser(spec::Parameters parameters)
	: m_context(parameters.program, std::move(parameters.parameters), {}, {}, parameters.custom_printer) {}

Parser::Parser(spec::Commands commands)
	: m_context(commands.program,
				std::move(commands.options),
				std::move(commands.commands),
				commands.policy,
				commands.custom_printer) {}

auto Parser::parse_args(std::span<std::string_view const> args) -> Result {
	m_context.reset_environment();
	auto program_parser = detail::ProgramParser{m_context};
	return program_parser.parse(args);
}
} // namespace clap
