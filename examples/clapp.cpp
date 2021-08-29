#include <algorithm>
#include <iostream>
#include <clap/clap.hpp>

namespace {
// primary parser
struct primary : clap::option_parser {
	enum flag { f_verbose };

	bool verbose{};

	primary() {
		static constexpr clap::option opt = {f_verbose, "verbose", "verbose mode"};
		spec.options = opt;
	}

	bool operator()(clap::option_key key, clap::str_t arg, clap::parse_state state) override {
		switch (key) {
		case f_verbose: verbose = true; return true;
		case no_end: return true;
		default: return false;
		}
	}
};

// parser for "greet"
struct greet : clap::option_parser {
	clap::str_t name;
	bool invoked{};

	greet() {
		spec.arg_doc = "NAME";
		spec.cmd_key = "greet";
		spec.cmd_doc = "Prints a greeting";
	}

	bool operator()(clap::option_key key, clap::str_t arg, clap::parse_state state) override {
		invoked = true;
		switch (key) {
		case no_arg: {
			// check if this is first arg
			if (state.arg_index() == 0) {
				name = arg;
				return true;
			}
			// no other args are supported
			return false;
		}
		case no_end: {
			// ensure name was passed (as first arg)
			return state.arg_index() == 1;
		}
		default: return false;
		}
	}

	void operator()() const {
		if (!name.empty()) { std::cout << "clap says, \"hello " << name << "!\"\n"; }
	}
};

// parser for "reverse"
struct reverse : clap::option_parser {
	std::vector<clap::str_t> words;
	bool source{};
	bool invoked{};

	reverse() {
		static constexpr clap::option opt('s', "source", "print source string");
		spec.options = opt;
		spec.arg_doc = "WORD0 [WORD1...]";
		spec.cmd_key = "reverse";
		spec.cmd_doc = "Reverses a string";
	}

	bool operator()(clap::option_key key, clap::str_t arg, clap::parse_state state) override {
		invoked = true;
		switch (key) {
		case 's': source = true; return true;
		case no_arg: {
			// store arbitrary number of words to reverse
			words.push_back(arg);
			return true;
		}
		case no_end: {
			// ensure at least one arg was passed (checking !words.empty() would also work here)
			return state.arg_index() > 0;
		}
		default: return false;
		}
	}

	void operator()() const {
		if (!words.empty()) {
			for (clap::str_t const str : words) {
				std::string s(str);
				std::reverse(s.begin(), s.end());
				if (source) { std::cout << '[' << str << "] => "; }
				std::cout << s << '\n';
			}
		}
	}
};
} // namespace

int main(int argc, char* argv[]) {
	primary primary;
	greet greet;
	reverse reverse;
	// prepare array of cmds
	clap::option_parser* cmds[] = {&greet, &reverse};
	clap::program_spec sp;
	sp.version = clap::lib_version();
	sp.doc = "clap example that demonstrates using cmds (multiple parsing contexts)";
	sp.parser = &primary;
	// set cmds in spec
	sp.cmds = cmds;
	// require at least one cmd to be parsed
	sp.cmds_required = 1;
	clap::parse_result const res = clap::parse_args(sp, argc, argv);
	switch (res) {
	case clap::parse_result::parse_error: return 10;
	case clap::parse_result::quit: return 0;
	default: break;
	}
	if (primary.verbose) {
		std::cout << "input:\n\tverbose\t\t: " << std::boolalpha << primary.verbose;
		std::cout << "\n\tgreet invoked\t: " << std::boolalpha << greet.invoked;
		std::cout << "\n\treverse invoked\t: " << std::boolalpha << reverse.invoked << "\n\n";
	}
	greet();
	reverse();
	std::cout << "\nPowered by clap ^^ [-v" << clap::lib_version() << ']' << std::endl;
}
