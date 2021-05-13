## Command Line Arguments Parser

A lightweight, header-only library utilising C++17.

> _Note: the shared name with [clap-rs/clap](https://github.com/clap-rs/clap) is pure coincidence!_

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/7b4a9afebf9b45809207eda599b66326)](https://www.codacy.com/gh/karnkaul/clap/dashboard?utm_source=github.com&utm_medium=referral&utm_content=karnkaul/clap&utm_campaign=Badge_Grade)

### Features

- Supports any char type (templated) and corresponding stdlib strings/views
- Helper class to interpret args and print help texts
- Customizable printer (via static polymorphism)
- CMake integration
- Example application (not built by default)

### Example

Refer to the [**example code**](example/clapp.cpp) for a program that prints the Nth fibonacci number. Configure CMake with `CLAP_EXAMPLE` to build it. Examples of its usage:

```
$ ./clapp
clapp ^^

$ ./clapp -h

Usage: clapp [OPTIONS...] [COMMAND] [ARGS...]

OPTIONS
      --hi         Print greeting
  -h, --help       Show help text
      --version    Show version

COMMANDS
  fib    Print Nth Fibonacci number


$ ./clapp --version
1.0

$ ./clapp --hi
hi!
clapp ^^

$ ./clapp fib -h

Usage: clapp fib [OPTIONS...] [ARGS...]

DESCRIPTION
  Print Nth Fibonacci number

OPTIONS
  -s, --sequence    Print entire sequence of N numbers
  -a, --a=[A]       Start number A
  -b, --b=[B]       Start number B

ARGUMENTS
  [N]


$ ./clapp fib -s
10th Fibonacci number (0, 1): 0 1 1 2 3 5 8 13 21 34
clapp ^^

$ ./clapp fib -s -a=1 -b=2 5
5th Fibonacci number (1, 2): 1 2 3 5 8
clapp ^^
```

### Usage

**Requirements**

- CMake
- C++17 compiler (and stdlib)

**Steps**

1. Clone repo to appropriate subdirectory, say `clap`
1. Add library to project via: `add_subdirectory(clap)` and `target_link_libraries(foo clap::clap)`
1. Use via: `#include <clap/parser.hpp>` or `#include <clap/interpreter.hpp>`
1. Configure with `CLAP_EXAMPLE=ON` to build the executable in `example`

#### parser

Takes in raw arguments and returns a schema of `expr_t`.

**Input Format**

```
[MAIN_OPTS...] [COMMAND] [CMD_OPTS...] [ARGUMENTS...]
```

All parameters are optional. Options are prefixed with `-` (single letter, supports chaining) or `--` (full word), and can take values separated by `=`. Arguments are collected verbatim.

**Expression Schema**

```
command
  id
  options[]
    id
    value
options[]
  id
  value
arguments[]
```

Note: arguments are common between main/global and command.

#### interpreter

Takes in the output of `parser` (`expr_t`) and a specification, interprets it to print appropriate help text and execute matched callbacks, and returns an enum specifying if the app should exit or resume. Uses `printer` by default to delegate actual printing, a custom type can also be used. `printer` provides an interface to print `expr_t`s as well.

An expression may invoke two callbacks: in `main` and a matched command (if any). The callbacks are passed a `params_t` representing matched options and verbatim arguments.

**Spec Format**

```
main
  exe
  version
  arg_format
  options[]
    id
    description
    value_format
  callback
commands{id => command}
  description
  arg_format
  options[]
    id
    description
    value_format
  callback
```

### Contributing

Pull/merge requests are welcome.

**[Original Repository](https://github.com/karnkaul/clap)**
