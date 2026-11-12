import clap;

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
	if (result.should_early_exit()) {
		return result.return_code();
	}

	if (verbose) {
		std::print("{} x {} = ", input, input);
	}
	auto const output = input * input;
	std::println("{}", output);

	return 0;
}
} // namespace

int main(int argc, char** argv) {
	try {
		if (argc < 1) {
			std::println(std::cerr, "argc is < 1 ({})", argc);
			return 1;
		}
		return run(argc, argv);
	} catch (std::exception const& e) {
		std::println(std::cerr, "PANIC: {}", e.what());
		return 1;
	} catch (...) {
		std::println("PANIC!");
		return 1;
	}
}
