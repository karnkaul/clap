## Command Line Arguments Parser

A lightweight, header-only library utilising C++17.

> _Note: the shared name with [clap-rs/clap](https://github.com/clap-rs/clap) is pure coincidence!_

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/7b4a9afebf9b45809207eda599b66326)](https://www.codacy.com/gh/karnkaul/clap/dashboard?utm_source=github.com&utm_medium=referral&utm_content=karnkaul/clap&utm_campaign=Badge_Grade)

### Spotlight: LittleEngineVk

```
$ ./levk-demo -h

Usage: levk-demo [OPTIONS...] [COMMAND] [ARGS...]

OPTIONS
      --vsync         Force VSYNC
      --validation    Force validation layers on/off
  -h, --help          Show help text
      --version       Show version

COMMANDS
  gpu    List / select from available GPUs

$ ./levk-demo gpu --help

Usage: levk-demo gpu [OPTIONS...] [ARGS...]

DESCRIPTION
  List / select from available GPUs

ARGUMENTS
  [index]

$ ./levk-demo --version
0.0.9
```

### Features

- Supports any char type (templated) and corresponding stdlib strings/views
- Helper class to interpret args and print help texts
- Customizable printer (via static polymorphism)
- CMake integration

### Usage

**Requirements**

- CMake
- C++17 compiler (and stdlib)

**Steps**

1. Clone repo to appropriate subdirectory, say `clap`
1. Add library to project via: `add_subdirectory(clap)` and `target_link_libraries(foo clap::clap)`
1. Use via: `#include <clap/parser.hpp>` or `#include <clap/helper.hpp>`

#### parser

Takes in raw arguments and returns a schema of `expr_t`.

**Input Format**

```
[MAIN_OPTS...] [COMMAND] [CMD_OPTS...] [ARGUMENTS...]
```

All parameters are optional. Options are prefixed with `-` (single letter, supports chaining) or `--` (full word), and can take values separated by `=` or space. Arguments are collected verbatim.

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

#### helper

Takes in the output of `parser` (`expr_t`) and a specification for help text, interprets it to print appropriate help text, and returns an enum specifying if the app should exit or resume. Uses `printer` by default to delegate actual printing, a custom type can be used as well. `printer` can be used to print `expr_t`s as well.

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
commands{id => comnand}
  description
  arg_format
  options[]
    id
    description
    value_format
```

**Example**

WIP

### Contributing

Pull/merge requests are welcome.

**[Original Repository](https://github.com/karnkaul/clap)**
