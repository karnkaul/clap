#include <algorithm>
#include <filesystem>
#include <iostream>
#include <clap/interpreter.hpp>

int fib(int a, int b, int n, bool print) {
	if (print) {
		std::cout << a << ' ';
	}
	return n > 0 ? fib(b, a + b, n - 1, print) : b;
}

/*

Shell output:

./clapp fib -s -a=1 -b=2 5
5th Fibonacci number (1, 2): 1 2 3 5 8
clapp ^^

*/

int main(int argc, char* argv[]) {
	clap::interpreter::spec_t spec;
	std::filesystem::path const exe = argv[0]; // store executable path
	spec.main.exe = exe.filename().string();   // set to executable filename
	spec.main.version = "1.0";				   // set version
	clap::interpreter::spec_t::opt_t opt_hi;
	opt_hi.id = "hi";					   // set full string option identifier
	opt_hi.description = "Print greeting"; // set option description
	spec.main.options.push_back(opt_hi);   // add option to main (global)
	// set main (global) callback
	spec.main.callback = [&opt_hi](clap::interpreter::params_t const& p) {
		if (p.opt_value(opt_hi.id)) {
			std::cout << "hi!" << std::endl;
		}
	};

	clap::interpreter::spec_t::cmd_t fib_cmd;			// construct new command
	fib_cmd.description = "Print Nth Fibonacci number"; // set command description
	fib_cmd.args_fmt = "[N]";							// set format for arguments
	clap::interpreter::spec_t::opt_t opt_seq;
	opt_seq.id = "sequence";
	opt_seq.description = "Print entire sequence of N numbers";
	fib_cmd.options.push_back(opt_seq); // add option to command
	clap::interpreter::spec_t::opt_t opt_a;
	opt_a.id = "a";
	opt_a.description = "Start number A";
	opt_a.value_fmt = "[A]"; // set value format for option
	fib_cmd.options.push_back(opt_a);
	clap::interpreter::spec_t::opt_t opt_b;
	opt_b.id = "b";
	opt_b.description = "Start number B";
	opt_b.value_fmt = "[B]";
	fib_cmd.options.push_back(opt_b);
	// set command callback
	fib_cmd.callback = [&opt_a, &opt_b, &opt_seq](clap::interpreter::params_t const& env) {
		bool seq = false;
		int a = 0;
		int b = 1;
		int n = 10;
		if (auto val = env.opt_value(opt_a.id)) {
			a = std::stoi(*val); // override a
		}
		if (auto opt = env.opt_value(opt_b.id)) {
			b = std::stoi(*opt); // override b
		}
		if (env.opt_value(opt_seq.id)) {
			seq = true; // set seq
		}
		if (!env.arguments.empty()) {
			n = std::max(std::stoi(env.arguments[0]), 2); // override n
		}
		// execute
		std::cout << n << "th Fibonacci number (" << a << ", " << b << "): " << fib(a, b, n - 2, seq) << std::endl;
	};
	spec.commands["fib"] = std::move(fib_cmd); // associate command with identifier string
	clap::interpreter interpreter;
	// parse arguments into expression
	auto const expr = interpreter.parse(argc, argv);
	// interpret expression with respect to specification
	if (interpreter.interpret(std::cout, std::move(spec), expr) == clap::interpreter::result::quit) {
		return 0; // early exit (help / version displayed)
	}
	std::cout << "clapp ^^" << std::endl;
	return 0;
}
