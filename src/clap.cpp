#include <clap/clap.hpp>
#include <src/parser.hpp>
#include <cassert>
#include <filesystem>
#include <format>

namespace clap {
namespace fs = std::filesystem;

namespace {
struct Spec {
	std::string app_name{};
	std::string description{};
	std::string footer{};
	std::string version{};
	OptionSpec options{};

	static Option make_option(std::string group, std::string description) {
		auto ret = Option{.description = std::move(description)};
		if (group.size() > 2 && group[1] == ',') {
			ret.letter = group[0];
			ret.key = group.substr(2);
		} else {
			ret.key = std::move(group);
			if (ret.key.size() == 1) { ret.letter = ret.key[0]; }
		}
		return ret;
	}
};

std::uint32_t compute_width(Option const& option) {
	auto ret = static_cast<std::uint32_t>(option.key.size()) + 2;
	if (option.argument) {
		ret += static_cast<std::uint32_t>(option.argument.usage.size());
		if (!option.argument.required) { ret += 2; }
	}
	return ret;
}

std::uint32_t compute_max_width(std::span<Option const> options) {
	auto ret = std::uint32_t{};
	for (auto const& option : options) { ret = std::max(ret, compute_width(option)); }
	return ret;
}

void print_letter_to(std::string& out, char const letter, bool print_blanks) {
	if (letter) {
		std::format_to(std::back_inserter(out), "-{}, ", letter);
	} else if (print_blanks) {
		out += std::string(4, ' ');
	}
}

void print_key_to(std::string& out, Option const& option) {
	std::format_to(std::back_inserter(out), "--{}", option.key);
	if (option.argument) {
		auto const open = option.argument.required ? std::string_view{} : "[";
		auto const close = option.argument.required ? std::string_view{} : "]";
		std::format_to(std::back_inserter(out), "{}={}{}", open, option.argument.usage, close);
	}
}
} // namespace

struct Options::Impl {
	Spec spec{};

	void print_help() const {
		auto usage = std::format("Usage: {} ", spec.app_name);
		if (!spec.options.options.empty()) { std::format_to(std::back_inserter(usage), "[OPTION]... "); }
		for (auto const& positional : spec.options.positional) {
			assert(positional.argument);
			std::format_to(std::back_inserter(usage), "<{}> ", positional.argument.usage);
		}
		if (spec.options.unmatched.argument) { std::format_to(std::back_inserter(usage), "{}", spec.options.unmatched.argument.usage); }
		std::fprintf(stdout, "%s\n", usage.c_str());

		std::fprintf(stdout, "\n%s\n", spec.description.c_str());

		if (!spec.options.options.empty()) {
			std::fprintf(stdout, "\nOPTIONS\n\n");
			auto const max_width = compute_max_width(spec.options.options) + 12;
			for (auto const& option : spec.options.options) {
				auto text = std::string{"  "};
				print_letter_to(text, option.letter, true);
				print_key_to(text, option);

				text += std::string(static_cast<std::size_t>(max_width) - text.size(), ' ');
				std::format_to(std::back_inserter(text), "{}", option.description);
				std::fprintf(stdout, "%s\n", text.c_str());
			}
		}

		if (!spec.footer.empty()) { std::fprintf(stdout, "\n%s\n", spec.footer.c_str()); }
	}
};

void Options::Deleter::operator()(Impl const* ptr) const { delete ptr; }

Options::Options(std::string app_name, std::string description, std::string version) : m_impl(new Impl{}) {
	m_impl->spec = {
		.app_name = std::move(app_name),
		.description = std::move(description),
		.version = std::move(version),
	};
}

Options& Options::set_footer(std::string footer) {
	m_impl->spec.footer = std::move(footer);
	return *this;
}

Options& Options::flag(bool& out_flag, std::string group, std::string description) {
	auto flag = Spec::make_option(std::move(group), std::move(description));
	flag.out_was_passed = &out_flag;
	if (!flag.key.empty()) { m_impl->spec.options.options.push_back(std::move(flag)); }
	return *this;
}

Options& Options::bind_option(bool* out_was_passed, detail::Argument argument, std::string group, std::string description) {
	auto option = Spec::make_option(std::move(group), std::move(description));
	if (option.key.empty()) { return *this; }
	option.out_was_passed = out_was_passed;
	if (argument.from_string) { option.argument = std::move(argument); }
	m_impl->spec.options.options.push_back(std::move(option));
	return *this;
}

Options& Options::bind_positional(detail::FromString from_string, std::string key, std::string usage) {
	auto positional = Option{.key = std::move(key)};
	if (positional.key.empty()) { return *this; }
	if (usage.empty()) { usage = positional.key; }
	positional.argument = detail::Argument{.usage = std::move(usage), .from_string = std::move(from_string)};
	m_impl->spec.options.positional.push_back(std::move(positional));
	return *this;
}

Options& Options::bind_unmatched(detail::FromString from_string, std::string usage) {
	m_impl->spec.options.unmatched.argument = detail::Argument{.usage = std::move(usage), .from_string = std::move(from_string)};
	return *this;
}

Result Options::parse(std::span<char const* const> args) {
	struct {
		bool help{};
		bool version{};
	} data{};
	auto parser = Parser{args, std::move(m_impl->spec.options)};
	auto option = Spec::make_option("help", "show help");
	option.out_was_passed = &data.help;
	parser.spec.options.push_back(std::move(option));
	option = Spec::make_option("version", "show version");
	option.out_was_passed = &data.version;
	parser.spec.options.push_back(std::move(option));

	try {
		while (parser.parse_next())
			;
		if (data.help || data.version) {
			if (data.help) {
				m_impl->spec.options = std::move(parser.spec);
				m_impl->print_help();
			} else {
				auto const msg = std::format("{} version {}\n", m_impl->spec.app_name, m_impl->spec.version);
				std::fprintf(stdout, "%s", msg.c_str());
			}
			return Result::eExit;
		}
		if (parser.position < parser.spec.positional.size()) { throw Error{"missing {} operand", parser.spec.positional[parser.position].key}; }
	} catch (Error const& error) {
		auto const msg = std::format("{}: {}\nTry '{} --help' for more information.\n", m_impl->spec.app_name, error.what(), m_impl->spec.app_name);
		std::fprintf(stderr, "%s", msg.c_str());
		return Result::eParseError;
	}

	return Result::eContinue;
}

Result Options::parse(int argc, char const* const* argv, bool const parse_arg0) {
	auto span = std::span{argv, static_cast<std::size_t>(argc)};
	if (!parse_arg0 && !span.empty()) { span = span.subspan(1); }
	return parse(span);
}
} // namespace clap

std::string clap::make_app_name(std::string_view const arg0) { return fs::path{arg0}.stem().string(); }
