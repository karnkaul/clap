# clap

**A lightweight Command Line Argument Parser library written in C++20**.

> _Note: the shared name with [clap-rs/clap](https://github.com/clap-rs/clap) is pure coincidence!_

## Quick start

This is a lightweight, type-safe, C++ option parser library, supporting the standard GNU style syntax for options.

Options can be given as:

```
-a
 ^
-ab
 ^^
--long
  ^
-abc=argument
 ^^^ ~~~~~~~~
-abc argument
 ^^^ ~~~~~~~~
-abcargument
 ^^^~~~~~~~~
--long=argument
  ^    ~~~~~~~~
--long argument
  ^    ~~~~~~~~
```

where `^` indicates an option and `~~~...` an argument. Here, `c` takes an argument while `a` and `b` do not.

Additionally, anything after `--` will be parsed as a positional argument.

## Usage

```cpp
#include <clap/options.hpp>
```

Create a `clap::Options` instance.

```cpp
auto options = clap::Options{
  "program name",
  "description of program",
  "version string",
};
```

Options are set up by passing a binding reference and the relevant metadata.

```cpp
struct {
  bool debug{};
  int log_level{3};

  struct {
    std::string field{"name"};
    bool enabled{};
  } sort{};

} input{};

options
  // bind an option that's a boolean flag
  .flag(input.debug, "d,debug", "enable debugging")
  // bind an option that requires an int argument
  .required(input.log_level, "log-level", "set the log level", "3")
  // bind an option that takes an optional string argument
  .optional(input.sort.field, input.sort.enabled, "s,sort", "sort by", "name|date");
```

Options are declared with a long and an optional short option. A description must be provided. Any type can be given as long as it can be parsed with `operator>>`. Options taking arguments also require usage text. Options with optional arguments require an additional `bool` flag that's set when the option is passed, regardless of the presence of the argument.

To parse the command line (and set all the bound references to parsed incoming arguments):

```cpp
auto result = options.parse(argc, argv);
```

The returned result is simply an enum, and can be used to check whether to continue execution and what code to return:

```cpp
if (clap::should_quit(result)) { return clap::return_code(result); }
```

### Lists of arguments

Options taking arguments can be bound to `std::vector`s if multiple options are expected to be passed. The vector will be filled in order of the occurrence of the option.

```cpp
auto defines = std::vector<std::string>{};

options.required(defines, "D,define", "Pass a preprocessor define", "VALUE");
```

### Positional arguments

Positional arguments are those given without a preceding flag and can be used alongside non-positional arguments. Set up positional arguments in order:

```cpp
struct {
  std::string source{};
  std::string destination{};
} args{};

options
  .positional(args.source, "src")
  .positional(args.destination, "dst");
```

Sample input:

```
program a.txt out/b.txt
        ^~~~^ ^~~~~~~~^
         src     dst
```

### Unmatched arguments

By default, arguments not matched to any bound option will result in an error. To instead accept them into an ordered vector:

```cpp
auto paths = std::vector<std::string>{};
options.unmatched(paths, "paths");
```

### Built-in options

These options are added on a call to `parse()`, and handled by the library:

```
--help
--version
```

### Gotchas

1. Adding options with duplicate / repeated keys causes undefined behaviour.
    1. This includes built-in options.
1. All references bound to an `Options` object must outlive all calls to `parse()` on it.

## Adding to a project

### Requirements

1. CMake 3.23+
1. C++20 compiler and standard library

### Summary

`clap` supports the following ways of integration:

1. Vendored in a project.
    1. Recommended: use CMake `FetchContent`.
    1. Alternatively, use git submodules / an archive in the repository and `add_subdirectory(path/to/clap)`.
1. Installed separately.
    1. Recommended: use vcpkg to create an overlay-port with the latest version of `clap`.
    1. Use `find_package(clap CONFIG REQUIRED)` in project CMake.

However the package is sourced / located, use `target_link_libraries(your-target [PRIVATE|PUBLIC] clap::clap)` to link. No other steps are needed (eg setting include paths).

### vcpkg example

Create the following files:

```
repo_root/
  vcpkg.json
  vcpkg/ports
    clap/
      portfile.cmake
      vcpkg.json
```

vcpkg.json:

```json
{
  "name": "test",
  "version": "0.1",
  "dependencies": [
    "clap"
  ],
  "vcpkg-configuration": {
    "overlay-ports": [
      "./vcpkg/ports/clap"
    ]
  }
}
```

vcpkg/ports/clap/vcpkg.json:

```json
{
  "name": "clap",
  "version": "0.4.2",
  "description": "command line argument parser",
  "dependencies": [
    {
      "name": "vcpkg-cmake",
      "host": true
    }
  ]
}
```

vcpkg/ports/clap/portfile.cmake:

```cmake
# setup the source metadata
vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO karnkaul/clap
  REF cmake-package
  SHA512 0 # obtain the SHA by running vcpkg-install
  HEAD_REF main
)

# configure through CMake
vcpkg_cmake_configure(SOURCE_PATH ${SOURCE_PATH})

# install through CMake
vcpkg_cmake_install()

# install copyright (required by vcpkg)
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/clap" RENAME copyright)

# remove unnecessary files (otherwise vcpkg will warn about them)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
```

Make sure to pass the path to your locally installed vcpkg toolchain file. Through CMake presets:

```json
"configurePresets": [
  {
    "name": "default",
    "toolchainFile": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
  }
]
```

## Example

Check out the swanky fibonacci number printer [example](example/fib.cpp).

## Contributing

Pull/merge requests are welcome.

**[Original Repository](https://github.com/karnkaul/clap)**
