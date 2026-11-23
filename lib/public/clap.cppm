export module clap;

export import clap.spec;
export import clap.build_version;

export namespace clap {
using Specification = std::variant<spec::Parameters, spec::Commands>;

[[nodiscard]] auto parse_args(Specification spec, std::span<std::string_view const> args) -> Result;
[[nodiscard]] auto parse_main(Specification spec, int argc, char const* const* argv) -> Result;
} // namespace clap
