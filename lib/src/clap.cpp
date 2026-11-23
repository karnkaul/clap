module clap;

import clap.parser;

namespace clap {
namespace {
void fixup_program_name(Specification& out_spec, std::string& out_buf, std::string_view const arg0) {
	if (arg0.empty()) {
		return;
	}

	auto const visitor = [&](auto& spec) {
		if (!spec.program.name.empty()) {
			return;
		}

		out_buf = to_program_name(arg0);
		spec.program.name = out_buf;
	};
	std::visit(visitor, out_spec);
}
} // namespace
} // namespace clap

auto clap::parse_args(Specification spec, std::span<std::string_view const> args) -> Result {
	auto const visitor = Visitor{
		[](spec::Parameters parameters) {
			return Parser{std::move(parameters)};
		},
		[](spec::Commands commands) {
			return Parser{std::move(commands)};
		},
	};
	auto parser = std::visit(visitor, std::move(spec));
	return parser.parse_args(args);
}

auto clap::parse_main(Specification spec, int const argc, char const* const* argv) -> Result {
	auto args = std::span{argv, static_cast<std::size_t>(argc)};
	auto program_name = std::string{};
	if (!args.empty()) {
		fixup_program_name(spec, program_name, args.front());
		args = args.subspan(1);
	}

	auto words = std::vector<std::string_view>{};
	words.reserve(args.size());
	for (auto const* arg : args) {
		words.emplace_back(arg);
	}
	return parse_args(std::move(spec), words);
}
