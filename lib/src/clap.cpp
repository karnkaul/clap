module clap;

import clap.parser;

namespace clap {
namespace {
[[nodiscard]] auto create_parser(Specification spec) {
	auto const visitor = [](auto t) {
		return Parser{std::move(t)};
	};
	return std::visit(visitor, std::move(spec));
}
} // namespace
} // namespace clap

auto clap::parse_words(Specification spec, std::span<std::string_view const> words) -> Result {
	auto parser = create_parser(std::move(spec));
	return parser.parse_words(words);
}

auto clap::parse_main(Specification spec, int const argc, char const* const* argv, bool const skip_argv_0) -> Result {
	auto parser = create_parser(std::move(spec));
	return parser.parse_main(argc, argv, skip_argv_0);
}

auto clap::parse_line(Specification spec, std::string_view const line) -> Result {
	auto parser = create_parser(std::move(spec));
	return parser.parse_line(line);
}
