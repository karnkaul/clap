export module clap;

export import clap.spec;
export import clap.build_version;

export namespace clap {
using Specification = std::variant<spec::Parameters, spec::Commands>;

/// \returns Result of parsing passed words.
[[nodiscard]] auto parse_words(Specification spec, std::span<std::string_view const> words) -> Result;
/// \returns Result of parsing args to main as words.
[[nodiscard]] auto parse_main(Specification spec, int argc, char const* const* argv, bool skip_argv_0 = true) -> Result;
/// \returns Result of parsing passed line split into words.
[[nodiscard]] auto parse_line(Specification spec, std::string_view line) -> Result;
} // namespace clap
