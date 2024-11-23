#include <clap/parse.hpp>
#include <array>
#include <fstream>
#include <print>

namespace {
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor,cppcoreguidelines-special-member-functions)
struct Command {
	auto operator=(Command&&) = delete;

	[[nodiscard]] virtual auto get_name() const -> std::string_view = 0;
	[[nodiscard]] virtual auto get_help() const -> std::string_view = 0;

	[[nodiscard]] virtual auto get_args() const -> std::span<clap::Arg const> = 0;

	virtual auto execute(bool debug) -> int = 0;
};

// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
struct Factorial : Command {
	[[nodiscard]] auto get_name() const -> std::string_view final { return "factorial"; }
	[[nodiscard]] auto get_help() const -> std::string_view final { return "print the factorial of an integer"; }

	[[nodiscard]] auto get_args() const -> std::span<clap::Arg const> final { return args; }

	auto execute(bool const debug) -> int final {
		if (debug) { std::println("params:\n  num\t: {}\n", num); }

		if (num < 0) {
			std::println(stderr, "invalid num: {}", num);
			return EXIT_FAILURE;
		}

		if (num > 20) {
			std::println("num too large: {}", num);
			return EXIT_FAILURE;
		}

		auto result = std::int64_t{1};
		for (auto current = std::int64_t(num); current > 1; --current) { result *= current; }

		std::println("factorial of {} is {}", num, result);
		return EXIT_SUCCESS;
	}

	int num{};

	std::array<clap::Arg, 1> args{
		clap::positional(num, clap::ArgType::Required, "NUM", "non-negative integer"),
	};
};

// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
struct Linecount : Command {
	[[nodiscard]] auto get_name() const -> std::string_view final { return "linecount"; }
	[[nodiscard]] auto get_help() const -> std::string_view final { return "count the lines in a file"; }

	[[nodiscard]] auto get_args() const -> std::span<clap::Arg const> final { return args; }

	auto execute(bool const debug) -> int final {
		if (debug) { std::println("params:\n  path\t: {}", path); }

		if (path.empty()) {
			std::println(stderr, "empty path");
			return EXIT_FAILURE;
		}

		auto file = std::ifstream{path.data()}; // path is null terminated
		if (!file) {
			std::println(stderr, "failed to open file: '{}'", path);
			return EXIT_FAILURE;
		}

		auto result = std::int64_t{};
		for (auto line = std::string{}; std::getline(file, line);) { ++result; }

		std::println("line count of '{}': {}", path, result);
		return EXIT_SUCCESS;
	}

	std::string_view path{};

	std::array<clap::Arg, 1> args{
		clap::positional(path, clap::ArgType::Required, "PATH", "path to input file"),
	};
};

auto run(int argc, char const* const* argv) -> int {
	static constexpr auto app_info_v = clap::AppInfo{
		.help_text = "multiple commands",
		.version = clap::version_v,
	};

	auto debug = false;
	auto factorial = Factorial{};
	auto linecount = Linecount{};
	auto const args = std::array{
		clap::flag(debug, "d,debug", "print parameters"),

		clap::command(factorial.get_args(), factorial.get_name(), factorial.get_help()),
		clap::command(linecount.get_args(), linecount.get_name(), linecount.get_help()),
	};

	auto const parse_result = clap::parse(app_info_v, args, argc, argv);
	if (parse_result.early_return()) { return parse_result.get_return_code(); }

	auto const cmd_name = parse_result.get_command_name();
	if (cmd_name == factorial.get_name()) { return factorial.execute(debug); }
	if (cmd_name == linecount.get_name()) { return linecount.execute(debug); }

	std::println(stderr, "unexpected command name: '{}'", cmd_name);
	return EXIT_FAILURE;
}
} // namespace

auto main(int argc, char** argv) -> int {
	try {
		return run(argc, argv);
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println(stderr, "FATAL ERROR");
		return EXIT_FAILURE;
	}
}
