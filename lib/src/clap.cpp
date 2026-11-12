module;

#include <cassert>

module clap;

import clap.detail;

namespace clap {
namespace fs = std::filesystem;

namespace {
[[nodiscard]] auto to_lower(char const in) {
	if (static_cast<int>(in) < 32 || static_cast<int>(in) >= 127) {
		return in;
	}
	auto const ret = std::tolower(static_cast<unsigned char>(in));
	if (ret < 0) {
		return '\0';
	}
	assert(ret <= int(std::numeric_limits<char>::max())); // NOLINT(misc-static-assert)
	return static_cast<char>(ret);
}

[[nodiscard]] auto iequals(std::string_view const a, std::string_view const b) {
	if (a.size() != b.size()) {
		return false;
	}
	auto const pred = [](auto const& zip) {
		auto const [a, b] = zip;
		return to_lower(a) == to_lower(b);
	};
	return std::ranges::all_of(std::views::zip(a, b), pred);
}

struct Printer : IPrinter {
	[[nodiscard]] static auto self() -> Printer& {
		static auto ret = Printer{};
		return ret;
	}

	void println(std::string_view const message) final { std::println("{}", message); }
	void printerr(std::string_view const message) final { std::println(std::cerr, "{}", message); }
};
} // namespace

namespace parameter {
auto Parse::operator()(std::string_view const input) const -> bool {
	if (was_set) {
		*was_set = true;
	}
	if (!func(input)) {
		return false;
	}
	return true;
}
} // namespace parameter

void parameter::split_named_key(std::string_view const key, char& out_letter, std::string_view& out_word) {
	if (key.empty()) {
		return;
	}

	if (key.size() == 1) {
		out_letter = key.front();
		return;
	}

	auto const comma = key.find(',');
	if (comma == std::string_view::npos) {
		out_word = key;
		return;
	}

	out_letter = key.front();
	out_word = key.substr(comma + 1);
}

auto parameter::parse_to(bool& out, std::string_view const input) -> bool {
	if (iequals(input, "true")) {
		out = true;
		return true;
	}

	if (iequals(input, "false")) {
		out = false;
		return true;
	}

	auto num = int{};
	if (!parse_to(num, input)) {
		return false;
	}

	out = static_cast<bool>(num);
	return true;
}

auto IPrinter::default_printer() -> IPrinter& { return Printer::self(); }
} // namespace clap

namespace clap {
Parser::Parser(spec::Parameters spec) : m_context(new detail::Context{std::move(spec)}) {}

Parser::Parser(spec::Commands spec) : m_context(new detail::Context{std::move(spec)}) {}

auto Parser::parse_words(std::span<std::string_view const> words) const -> Result {
	if (!m_context) {
		return Result::error(ExitCode::InvalidParser);
	}
	try {
		auto impl = detail::ParserImpl{static_cast<detail::Context&>(*m_context), words};
		auto const result = impl.parse();
		return Result{ExitCode::Success, result.outcome, result.command_identifier};
	} catch (detail::Error const error) {
		return Result::error(to_exit_code(error));
	}
}

auto Parser::parse_line(std::string_view const line) const -> Result {
	if (!m_context) {
		return Result::error(ExitCode::InvalidParser);
	}
	auto const words = to_words(line);
	return parse_words(words);
}

auto Parser::parse_main(int const argc, char const* const* argv, bool const skip_argv_0) const -> Result {
	if (!m_context) {
		return Result::error(ExitCode::InvalidParser);
	}
	auto span = std::span{argv, std::size_t(argc)};
	auto program_name = std::string{};
	if (skip_argv_0 && !span.empty()) {
		program_name = to_program_name(span.front());
		static_cast<detail::Context&>(*m_context).override_program_name_if_empty(program_name);
		span = span.subspan(1);
	}
	auto const words = std::vector<std::string_view>{span.begin(), span.end()};
	return parse_words(words);
}

auto Parser::get_help_text() const -> std::string { return detail::ParserImpl{static_cast<detail::Context&>(*m_context), {}}.help_text(); }
} // namespace clap

namespace clap {
namespace {
class WordScanner {
  public:
	explicit constexpr WordScanner(std::string_view const line) : m_remain(line) {}

	constexpr auto next(std::string_view& out) -> bool {
		trim_whitespace();
		if (m_remain.empty()) {
			return false;
		}
		if (m_remain.front() == '"') {
			out = scan_quoted();
			return true;
		}
		out = scan_unquoted();
		return true;
	}

  private:
	static constexpr auto is_whitespace(char const c) { return c == ' ' || c == '\t'; };

	constexpr void trim_whitespace() {
		while (!m_remain.empty() && is_whitespace(m_remain.front())) {
			m_remain.remove_prefix(1);
		}
	}

	constexpr auto scan_quoted() -> std::string_view {
		assert(!m_remain.empty() && m_remain.front() == '"'); // NOLINT(misc-static-assert)
		m_remain.remove_prefix(1);

		auto const i = m_remain.find('"');
		if (i == std::string_view::npos) {
			return std::exchange(m_remain, {});
		}

		auto const ret = m_remain.substr(0, i);
		m_remain.remove_prefix(i + 1);
		return ret;
	}

	constexpr auto scan_unquoted() -> std::string_view {
		for (auto length = 1uz; length < m_remain.size(); ++length) {
			char const c = m_remain[length]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			if (is_whitespace(c) || c == '"') {
				auto const ret = m_remain.substr(0, length);
				m_remain.remove_prefix(length);
				return ret;
			}
		}
		return std::exchange(m_remain, {});
	}

	std::string_view m_remain{};
};
} // namespace
} // namespace clap

auto clap::to_program_name(std::string_view const argv_0) -> std::string {
	return fs::absolute(argv_0).stem().string();
}

auto clap::to_words(std::string_view const line) -> std::vector<std::string_view> {
	auto ret = std::vector<std::string_view>{};
	auto scanner = WordScanner{line};
	for (auto word = std::string_view{}; scanner.next(word);) {
		ret.push_back(word);
	}
	return ret;
}

auto clap::command(std::string_view const identifier,
				   ParameterList parameters,
				   std::string_view const description,
				   std::string_view epilogue) -> Command {
	return Command{identifier, std::move(parameters), description, epilogue};
}
