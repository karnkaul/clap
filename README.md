## Command Line Arguments Parser

A lightweight command line argument parsing library written in C++17.

> _Note: the shared name with [clap-rs/clap](https://github.com/clap-rs/clap) is pure coincidence!_

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/7b4a9afebf9b45809207eda599b66326)](https://www.codacy.com/gh/karnkaul/clap/dashboard?utm_source=github.com&utm_medium=referral&utm_content=karnkaul/clap&utm_campaign=Badge_Grade)

### Features

- Inspired by `argp_parse` ([Argp](https://www.gnu.org/software/libc/manual/html_node/Argp.html))
- Supports option lists, non-options anywhere, and parsing context switching via "cmds"
- Customizable root parser (handles `--help` etc)
- CMake integration
- Example applications (not built by default)

### Limitations

- Non customizable printers (use a custom root parser and print everything yourself instead)

### Example

There are two examples demonstrating basic and advanced usage:

- [**fib**](examples/fib.cpp) prints the Nth fibonacci number
- [**clapp**](examples/clapp.cpp) greets name and/or reverses arbitrary number of strings (using multiple parsers / cmds)

Configure CMake with `CLAP_BUILD_EXAMPLES=ON` to build them.

```
$ ./fib
Usage: fib [OPTION...] N
Try 'fib --help' or 'fib --usage' for more information.

$ ./fib --help
Usage: fib [OPTION...] N
fib -- clap example that prints a fibonacci number / sequence

OPTIONS
  -s, --sequence                print full sequence
      --verbose                 verbose mode
  -a, --arg-a=A                 start value of a
  -b, --arg-b=B                 start value of b
      --help                    Show this help text
      --version                 Show version
      --usage                   Show usage

B must be > A

$ ./fib 5
5th Fibonacci number (0, 1): 3

Powered by clap ^^ [-v0.2]

$ ./fib --usage
Usage: fib [-s] [-aA] [-bB] [--sequence] [--verbose] [--arg-a=A] [--arg-b=B] [--help] [--version] [--usage] N

$ ./fib --verbose -s -a1 -b2 5

input:
        verbose         : true
        sequence        : true
        arg_a           : 1
        arg_b           : 2
        arg_n           : 5

5th Fibonacci number (1, 2): 1 2 3 5 8

Powered by clap ^^ [-v0.2]

$ ./fib --version
0.2
```

### Usage

**Requirements**

- CMake
- C++17 compiler (and stdlib)

**Steps**

1. Clone repo to appropriate subdirectory, say `clap`
1. Add library to project via: `add_subdirectory(clap)` and `target_link_libraries(foo clap::clap)`
1. Use via `#include <clap/clap.hpp>`
1. Configure with `CLAP_BUILD_EXAMPLES=ON` to build the executables in `examples` (also adds tests)

### Expression syntax

```
Syntax: [option...] [arg...] [cmd...]
	cmd: cmd_key [option...] [arg...]
	cmd_key (regex): [A-z]+
	option (no / optional argument): -keylist | -option_key[=arg] | --option_name[=arg]
	option (required argument): -option_keyarg | -option_key arg | -option_key=arg | --option_name=arg
	option_key (regex): [A-z]
	option_name (regex): [A-z]+
	keylist (only options without arguments): -option_key0[option_key1...]
```

### Contributing

Pull/merge requests are welcome.

**[Original Repository](https://github.com/karnkaul/clap)**
