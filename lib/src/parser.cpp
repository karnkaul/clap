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

auto Parser::parse_words(std::span<std::string_view const> words) -> Result {
	m_context.reset_environment();
	auto program_parser = detail::ProgramParser{m_context};
	return program_parser.parse(words);
}

auto Parser::parse_main(int argc, char const* const* argv, bool const skip_argv_0) -> Result {
	auto args = std::span{argv, static_cast<std::size_t>(argc)};
	if (skip_argv_0 && !args.empty()) {
		m_context.fixup_program_name(args.front());
		args = args.subspan(1);
	}

	auto const words = to_words(args);
	return parse_words(words);
}

auto Parser::parse_line(std::string_view const line) -> Result {
	auto const words = to_words(line);
	return parse_words(words);
}
} // namespace clap
