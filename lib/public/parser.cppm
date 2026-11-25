export module clap.parser;

export import clap.spec;

import clap.detail;

export namespace clap {
class Parser {
  public:
	/// \brief Create a Parser for a list of Parameters.
	explicit Parser(spec::Parameters parameters);
	/// \brief Create a Parser for a list of Commands.
	explicit Parser(spec::Commands commands);

	/// \returns Result of parsing passed words.
	[[nodiscard]] auto parse_words(std::span<std::string_view const> words) -> Result;
	/// \returns Result of parsing args to main as words.
	[[nodiscard]] auto parse_main(int argc, char const* const* argv, bool skip_argv_0 = true) -> Result;
	/// \returns Result of parsing passed line split into words.
	[[nodiscard]] auto parse_line(std::string_view line) -> Result;

  private:
	detail::ProgramContext m_context;
};
} // namespace clap
