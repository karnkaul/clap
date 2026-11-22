#pragma once

import clap.detail;

namespace clap::test {
class MockContext : public detail::IContext {
  public:
	void print_help_text() final { m_help_printed = true; }
	[[nodiscard]] auto has_version() const -> bool final { return m_has_version; }
	void print_version() final { m_version_printed = true; }

	void print_error(std::string_view message) final { m_error = message; }

	bool m_help_printed{};
	bool m_has_version{};
	bool m_version_printed{};
	std::string m_error{};
};

[[nodiscard]] inline auto to_tokens(std::string_view const input) {
	auto const words = to_words(input);
	auto ret = std::vector<ScannerToken>{};
	ret.reserve(words.size());
	auto scanner = Scanner{words};
	auto token = ScannerToken{};
	while (scanner.scan_next(token)) {
		ret.push_back(token);
	}
	return ret;
}
} // namespace clap::test
