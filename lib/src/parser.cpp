module clap.parser;

namespace clap {
Parser::Parser(spec::Parameters parameters) : m_context(parameters.program, parameters.custom_printer) {
	auto const result = m_environment.build(
		m_context.get_error_printer(), parameters.program, std::move(parameters.parameters), {}, {});
	if (result != ExitCode::Success) {
		// TODO
	}
}

Parser::Parser(spec::Commands commands) : m_context(commands.program, commands.custom_printer) {
	auto const result = m_environment.build(m_context.get_error_printer(),
											commands.program,
											std::move(commands.options),
											std::move(commands.commands),
											commands.policy);
	if (result != ExitCode::Success) {
		// TODO
	}
}

auto Parser::parse_args(std::span<std::string_view const> args) -> Result {
	auto program_parser = detail::ProgramParser{m_context, m_environment};
	return program_parser.parse(args);
}
} // namespace clap
