#include "clap/build_version.hpp"
#include "clap/parameter.hpp"
#include "clap/parser.hpp"
#include "clap/spec.hpp"
#include <cstdlib>
#include <print>

namespace {
[[nodiscard]] auto run(int const argc, char const* const* argv) {
	auto spec = clap::spec::Parameters{
		.program =
			{
				.version = clap::build_version_v,
				.description = "print the square of an integer",
			},
	};
	auto input = int{};
	auto verbose = bool{};
	spec.parameters = {
		clap::named_flag(verbose, "v,verbose", "print full expression"),
		clap::positional_required(input, "NUM", "integer to square"),
	};

	auto parser = clap::Parser{std::move(spec)};
	auto const result = parser.parse_main(argc, argv);
	if (result.should_early_exit()) { return result.return_code(); }

	if (verbose) { std::print("{} x {} = ", input, input); }
	auto const output = input * input;
	std::println("{}", output);

	return EXIT_SUCCESS;
}
} // namespace

int main(int argc, char** argv) {
	try {
		if (argc < 1) {
			std::println(stderr, "argc is < 1 ({})", argc);
			return EXIT_FAILURE;
		}
		return run(argc, argv);
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println("PANIC!");
		return EXIT_FAILURE;
	}
}
