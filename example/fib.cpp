#include <clap/clap.hpp>
#include <format>
#include <iostream>

namespace {
constexpr std::string_view suffix(int const num) {
	switch (num % 10) {
	case 1: return "st";
	case 2: return "nd";
	case 3: return "rd";
	default: return "th";
	}
}

// driver function
int fib(int a, int b, int n, bool print) {
	if (print) { std::cout << a << ' '; }
	return n > 0 ? fib(b, a + b, n - 1, print) : b;
}

// program input / cache
struct Input {
	int a = 0;
	int b = 1;
	int n = 0;
	bool sequence = false;
	bool verbose = false;
};
} // namespace

int main(int argc, char* argv[]) {
	// create an instance of Input to bind options to
	auto input = Input{};
	static constexpr auto app_description = "clap example that prints a fibonacci number / sequence";
	auto const version = std::format("v{}", clap::version_v);
	// pass the file stem of argv[0] as program_name
	auto options = clap::Options{clap::make_app_name(argv[0]), app_description, version};
	// setup options
	options
		// add boolean flags
		.flag(input.verbose, "v,verbose", "verbose mode")
		.flag(input.sequence, "s,sequence", "print full sequence")
		// add required int arguments
		.required(input.a, "a", "1st number in sequence", "0")
		.required(input.b, "b", "2nd number in sequence", "1")
		// add a (required) positional argument
		.positional(input.n, "n");

	// parse the incoming arguments
	auto const result = options.parse(argc, argv);
	// exit if desired (error or --help / --version handled)
	if (clap::should_quit(result)) { return clap::return_code(result); }

	// setup program state
	if (input.verbose) {
		std::cout << std::format("\ninput:\n\tverbose\t\t: {}\n\tsequence\t: {}", input.verbose, input.sequence)
				  << std::format("\n\ta\t\t: {}\n\tb\t\t: {}\n\tn\t\t: {}\n\n", input.a, input.b, input.n);
	}

	// compute result
	auto const answer = fib(input.a, input.b, input.n, input.sequence);
	if (input.sequence) { std::cout << "\n"; }
	// output result
	std::cout << std::format("{}{} Fibonacci number ({}, {}): {}\n", input.n, suffix(input.n), input.a, input.b, answer);

	std::cout << std::format("\nPowered by clap {} ^^\n", version);
}
