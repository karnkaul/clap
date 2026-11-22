module;

#include <cassert>

module clap.spec;

namespace clap {
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
} // namespace

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
} // namespace clap
