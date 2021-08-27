#include <iostream>
#include <clap/clap.hpp>

namespace {
// driver function
int fib(int a, int b, int n, bool print) {
	if (print) { std::cout << a << ' '; }
	return n > 0 ? fib(b, a + b, n - 1, print) : b;
}

// program input / cache
struct input {
	int a = 0;
	int b = 1;
	int n = 0;
	bool sequence = false;
	bool verbose = false;

	// safe / optional std::atoi
	static int get_if_unsigned(std::string_view str) noexcept {
		for (char const c : str) {
			if (!std::isdigit(c)) { return -1; }
		}
		return std::atoi(str.data());
	}
};

struct parser : clap::option_parser {
	// unscoped enum as integral keys for corresponding options
	enum flag { f_verbose };

	input in;

	parser() {
		// setup options; this can be a data member instead as well, it just needs to survive throughout the lifetime of this instance
		static constexpr clap::option opts[] = {
			{'s', "sequence", "print full sequence"},
			{f_verbose, "verbose", "verbose mode"},
			{'a', "arg-a", "start value of a", "A"},
			{'b', "arg-b", "start value of b", "B"},
		};
		spec.options = opts;
		// setup docs
		spec.doc_desc = "B must be > A";
		spec.arg_doc = "N";
	}

	bool operator()(clap::option_key key, clap::str_t arg, clap::parse_state state) {
		// compare passed keys against configured options in spec
		switch (key) {
		case 's': in.sequence = true; return true;
		case f_verbose: in.verbose = true; return true;
		case 'a': {
			// check if arg is an int
			if (int const a = input::get_if_unsigned(arg); a >= 0) {
				in.a = a;
				return true;
			}
			// else fail parsing
			return false;
		}
		case 'b': {
			// check if arg is an int
			if (int const b = input::get_if_unsigned(arg); b >= 0) {
				in.b = b;
				return true;
			}
			// else fail parsing
			return false;
		}
		case no_arg: {
			int const n = input::get_if_unsigned(arg);
			// ensure n is first argument
			if (n >= 0 && state.arg_index() == 0) {
				in.n = n;
				return true;
			}
			// else fail parsing
			return false;
		}
		case no_end: {
			// ensure n was passed (as first argument)
			return state.arg_index() > 0 && in.b > in.a;
		}
		// this should not be reachable; it implies one or more configured options were not handled
		default: return false;
		}
	}
};
} // namespace

int main(int argc, char* argv[]) {
	parser parser;
	// setup program spec
	clap::program_spec sp;
	sp.version = clap::lib_version();
	sp.doc = "clap example that prints a fibonacci number / sequence";
	sp.parser = &parser;
	// obtain parse result
	clap::parse_result const res = clap::parse_args(sp, argc, argv);
	// early return unless parser returned "run"
	switch (res) {
	case clap::parse_result::parse_error: return 10;
	case clap::parse_result::quit: return 0;
	default: break;
	}
	auto const& in = parser.in;
	if (in.verbose) {
		std::cout << "\ninput:\n\tverbose\t\t: " << std::boolalpha << in.verbose << "\n\tsequence\t: " << std::boolalpha << in.sequence;
		std::cout << "\n\targ_a\t\t: " << in.a << "\n\targ_b\t\t: " << in.b << "\n\targ_n\t\t: " << in.n << "\n\n";
	}
	// execute program based on parsed input
	std::cout << in.n << "th Fibonacci number (" << in.a << ", " << in.b << "): " << fib(in.a, in.b, in.n - 2, in.sequence) << std::endl;
	std::cout << "\nPowered by clap ^^ [-v" << clap::lib_version() << ']' << std::endl;
}
