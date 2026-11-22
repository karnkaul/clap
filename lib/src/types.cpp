module;

#include <cassert>

module clap.types;

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

namespace fs = std::filesystem;

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

auto clap::or_default(IPrinter* custom_printer) -> IPrinter& {
	if (custom_printer) {
		return *custom_printer;
	}

	static auto ret = DefaultPrinter{};
	return ret;
}
