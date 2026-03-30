#include "clap/build_version.hpp"
#include "clap/command.hpp"
#include "clap/parameter.hpp"
#include "clap/parser.hpp"
#include "clap/program.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <print>
#include <string_view>

namespace {
namespace fs = std::filesystem;

class ICommand {
  public:
	virtual ~ICommand() = default;

	ICommand() = default;
	ICommand(ICommand const&) = default;
	ICommand(ICommand&&) = default;
	ICommand& operator=(ICommand const&) = default;
	ICommand& operator=(ICommand&&) = default;

	[[nodiscard]] virtual auto get_identifier() const -> std::string_view = 0;
	[[nodiscard]] virtual auto get_description() const -> std::string_view = 0;
	[[nodiscard]] virtual auto get_parameters() -> std::vector<clap::Parameter> = 0;

	virtual auto execute() -> int = 0;
};

class LargestFile : public ICommand {
	[[nodiscard]] auto get_identifier() const -> std::string_view final { return "largest-file"; }
	[[nodiscard]] auto get_description() const -> std::string_view final { return "print the file with the largest size"; }

	[[nodiscard]] auto get_parameters() -> std::vector<clap::Parameter> final {
		return {
			clap::named_flag(m_verbose, "v,verbose", "list every file"),
			clap::positional_list(m_paths, "PATH", "list of file paths"),
		};
	}

	auto execute() -> int final {
		if (m_paths.empty()) { return EXIT_SUCCESS; }

		struct File {
			fs::path path{};
			std::int64_t size{};
		};
		auto files = std::vector<File>{};
		files.reserve(m_paths.size());
		for (auto const in : m_paths) {
			auto path = fs::path{in};
			if (!fs::is_regular_file(path)) {
				std::println(stderr, "not a file: '{}'", path.generic_string());
				return EXIT_FAILURE;
			}
			files.push_back(File{.path = path, .size = std::int64_t(fs::file_size(path))});
			if (m_verbose) { std::println("-- {} ({}B)", files.back().path.generic_string(), files.back().size); }
		}

		std::ranges::sort(files, [](File const& a, File const& b) { return a.size > b.size; });
		std::println("{}", files.front().path.generic_string());
		return EXIT_SUCCESS;
	}

	std::vector<std::string_view> m_paths{};
	bool m_verbose{};
};

class Pow : public ICommand {
	[[nodiscard]] auto get_identifier() const -> std::string_view final { return "pow"; }
	[[nodiscard]] auto get_description() const -> std::string_view final { return "raise a number to a power"; }

	[[nodiscard]] auto get_parameters() -> std::vector<clap::Parameter> final {
		return {
			clap::named_flag(m_verbose, "v,verbose", "print entire expression"),
			clap::positional_required(m_var, "VAR", "variable to raise"),
			clap::positional_optional(m_exp, "EXP (2)", "exponent to raise to"),
		};
	}

	auto execute() -> int final {
		if (m_verbose) { std::print("{}^{} = ", m_var, m_exp); }
		auto const output = std::pow(m_var, m_exp);
		std::println("{}", output);
		return EXIT_SUCCESS;
	}

	int m_var{};
	int m_exp{2};
	bool m_verbose{};
};

class App {
  public:
	explicit App() {
		m_commands.push_back(std::make_unique<LargestFile>());
		m_commands.push_back(std::make_unique<Pow>());
	}

	[[nodiscard]] auto run(int const argc, char const* const* argv) {
		auto const program_name = clap::to_program_name(*argv);
		auto const program = clap::Program{
			.name = program_name,
			.version = clap::build_version_v,
			.description = "print the square of an integer",
		};
		auto command_list = std::vector<clap::Command>{};
		command_list.reserve(m_commands.size());
		for (auto const& command : m_commands) {
			command_list.push_back(clap::command(command->get_identifier(), command->get_parameters(), command->get_description()));
		}
		auto parser = clap::Parser{std::move(command_list), {}, program};
		auto const result = parser.parse_main(argc, argv);
		if (result.should_early_exit()) { return result.return_code(); }

		auto const it = std::ranges::find_if(m_commands, [id = result.command_identifier()](auto const& command) { return command->get_identifier() == id; });
		if (it == m_commands.end()) {
			std::println(stderr, "unrecognized command: '{}'", result.command_identifier());
			return EXIT_FAILURE;
		}

		auto const& command = *it;
		return command->execute();
	}

  private:
	std::vector<std::unique_ptr<ICommand>> m_commands{};
};
} // namespace

int main(int argc, char** argv) {
	try {
		if (argc < 1) {
			std::println(stderr, "argc is < 1 ({})", argc);
			return EXIT_FAILURE;
		}
		return App{}.run(argc, argv);
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println("PANIC!");
		return EXIT_FAILURE;
	}
}
