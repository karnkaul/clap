export module clap.parser;

import clap.detail;

export namespace clap {
class Parser {
  public:
	explicit Parser(spec::Parameters parameters);
	explicit Parser(spec::Commands commands);

	[[nodiscard]] auto parse_args(std::span<std::string_view const> args) -> Result;

  private:
	detail::ProgramContext m_context;
};
} // namespace clap
