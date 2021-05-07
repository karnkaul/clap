#pragma once
#include <cassert>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <clap/parser.hpp>

namespace clap {
template <typename Ch = char>
class basic_helper;

using helper = basic_helper<>;

template <typename Ch>
class basic_helper : public basic_parser<Ch> {
  public:
	using char_t = Ch;
	using ostream_t = std::basic_ostream<char_t>;
	using typename basic_parser<char_t>::string_t;
	using typename basic_parser<char_t>::string_view_t;
	using typename basic_parser<char_t>::command_t;
	using typename basic_parser<char_t>::option_t;
	using typename basic_parser<char_t>::expr_t;

	enum class result { resume, quit };

	inline static constexpr char_t null_char{};

	struct spec_t;
	class printer_t;

	static bool option_match(string_view_t id, bool single, string_view_t str) noexcept;

	template <typename Pr = printer_t>
	result interpret(ostream_t& out, spec_t spec, expr_t const& expr) const;

  private:
	static void fill_opts(std::vector<typename spec_t::opt_t> const& options);
};

template <typename Ch>
struct basic_helper<Ch>::spec_t {
	struct opt_t {
		string_t id;
		string_t description;
		string_t value_fmt;
		mutable char_t single = null_char;
	};

	struct params_t {
		string_t arg_fmt;
		std::vector<opt_t> options;
	};

	struct cmd_t : params_t {
		string_t description;
	};

	struct main_t : params_t {
		string_t exe;
		string_t version;
	};

	using cmd_map_t = std::unordered_map<string_t, cmd_t>;

	main_t main;
	cmd_map_t commands;

	template <typename Cont>
	static std::size_t max_length(Cont const& strings) noexcept;
};

template <typename Ch>
class basic_helper<Ch>::printer_t {
  public:
	using cmd_t = typename spec_t::cmd_t;

	explicit printer_t(ostream_t* out) noexcept;

	string_view_t help_id() const noexcept;
	string_view_t version_id() const noexcept;
	void operator()(string_view_t version) const;
	void operator()(spec_t const& spec) const;
	void operator()(string_view_t main, string_view_t id, cmd_t const& cmd) const;

	void print(expr_t const& expr, result const* result = nullptr) const;

  private:
	struct stream;
	inline static constexpr std::size_t indent = 4;

	void commands(stream& out, typename spec_t::cmd_map_t const& commands) const;
	void options(stream& out, std::vector<typename spec_t::opt_t> const& options) const;
	void arguments(stream& out, string_view_t arg_fmt) const;

	template <typename T, typename F>
	static void printerate(stream& out, std::vector<T> const& vec, string_view_t title, F f);
	template <typename T, typename F>
	static void printerate(stream& out, std::vector<T> const& vec, F f);

	ostream_t* m_out{};
};

// impl

template <typename Ch>
struct basic_helper<Ch>::printer_t::stream : std::basic_stringstream<Ch> {
	explicit stream(ostream_t* out) : out(out) {
	}
	~stream() {
		(*out) << this->rdbuf() << std::endl;
	}

  private:
	ostream_t* out;
};

template <typename Ch>
basic_helper<Ch>::printer_t::printer_t(ostream_t* out) noexcept : m_out(out) {
	assert(m_out != nullptr);
}

template <typename Ch>
typename basic_helper<Ch>::string_view_t basic_helper<Ch>::printer_t::help_id() const noexcept {
	return "help";
}

template <typename Ch>
typename basic_helper<Ch>::string_view_t basic_helper<Ch>::printer_t::version_id() const noexcept {
	return "version";
}

template <typename Ch>
void basic_helper<Ch>::printer_t::operator()(string_view_t version) const {
	stream str(m_out);
	str << version;
}

template <typename Ch>
void basic_helper<Ch>::printer_t::operator()(spec_t const& spec) const {
	stream str(m_out);
	str << "\nUsage: ";
	if (!spec.main.exe.empty()) {
		str << spec.main.exe << ' ';
	}
	str << "[OPTIONS...] [COMMAND] [ARGS...]\n";
	options(str, spec.main.options);
	commands(str, spec.commands);
	arguments(str, spec.main.arg_fmt);
}

template <typename Ch>
void basic_helper<Ch>::printer_t::operator()(string_view_t main, string_view_t id, typename spec_t::cmd_t const& cmd) const {
	stream str(m_out);
	str << "\nUsage: ";
	if (!main.empty()) {
		str << main << ' ';
	}
	str << id << " [OPTIONS...] [ARGS...]\n\nDESCRIPTION\n  " << (cmd.description.empty() ? "[None]" : cmd.description) << '\n';
	options(str, cmd.options);
	arguments(str, cmd.arg_fmt);
}

template <typename Ch>
void basic_helper<Ch>::printer_t::print(expr_t const& expr, result const* result) const {
	stream str(m_out);
	str << "expression:";
	if (result) {
		str << "\n  result\t: " << (*result == result::quit ? "quit" : "resume");
	}
	printerate(str, expr.options, "  options", [&str](auto const& op) {
		str << op.id;
		if (!op.value.empty()) {
			str << '=' << op.value;
		}
	});
	if (!expr.command.id.empty()) {
		str << "\n  command\t: " << expr.command.id;
		printerate(str, expr.command.options, "    options", [&str](auto const& op) {
			str << op.id;
			if (!op.value.empty()) {
				str << '=' << op.value;
			}
		});
	}
	printerate(str, expr.arguments, "  arguments", [&str](auto const& arg) { str << arg; });
}

template <typename Ch>
void basic_helper<Ch>::printer_t::commands(stream& out, typename spec_t::cmd_map_t const& commands) const {
	if (!commands.empty()) {
		out << "\nCOMMANDS\n";
		std::vector<string_view_t> cmds;
		cmds.reserve(commands.size());
		for (auto const& [id, _] : commands) {
			cmds.push_back(id);
		}
		printerate(out, cmds, [&commands](auto, auto const& id) { return commands.find(string_t(id))->second.description; });
	}
}

template <typename Ch>
void basic_helper<Ch>::printer_t::options(stream& out, std::vector<typename spec_t::opt_t> const& options) const {
	if (!options.empty()) {
		out << "\nOPTIONS\n";
		std::vector<string_t> opts;
		opts.reserve(options.size());
		for (auto const& opt : options) {
			std::basic_stringstream<Ch> option;
			if (opt.single != null_char) {
				option << '-' << opt.id[0] << ", ";
			} else {
				option << std::setw(6);
			}
			option << "--" << opt.id;
			if (!opt.value_fmt.empty()) {
				option << '=' << opt.value_fmt;
			}
			opts.push_back(option.str());
		}
		printerate(out, opts, [&options](auto idx, auto const&) { return options[idx].description; });
	}
}

template <typename Ch>
void basic_helper<Ch>::printer_t::arguments(stream& out, string_view_t arg_fmt) const {
	if (!arg_fmt.empty()) {
		out << "\nARGUMENTS\n  " << arg_fmt << '\n';
	}
}

template <typename Ch>
template <typename T, typename F>
void basic_helper<Ch>::printer_t::printerate(stream& str, std::vector<T> const& vec, string_view_t title, F f) {
	bool first = true;
	if (!vec.empty()) {
		str << "\n" << title << "\t: ";
		first = true;
		for (auto const& t : vec) {
			if (!first) {
				str << ',';
			}
			first = false;
			f(t);
		}
	}
}

template <typename Ch>
template <typename T, typename F>
void basic_helper<Ch>::printer_t::printerate(stream& out, std::vector<T> const& c, F f) {
	auto const max_size = spec_t::max_length(c);
	std::size_t idx = 0;
	for (auto const& s : c) {
		auto const desc = f(idx, s);
		auto const description = desc.empty() ? string_view_t("-") : string_view_t(desc);
		std::size_t const width = max_size - s.size() + description.size() + indent;
		out << "  " << s << std::setw((int)width) << description << '\n';
		++idx;
	}
}

template <typename Ch>
template <typename Cont>
std::size_t basic_helper<Ch>::spec_t::max_length(Cont const& strings) noexcept {
	std::size_t ret = 0;
	for (auto const& str : strings) {
		if (str.size() > ret) {
			ret = str.size();
		}
	}
	return ret;
}

template <typename Ch>
template <typename Pr>
typename basic_helper<Ch>::result basic_helper<Ch>::interpret(ostream_t& out, spec_t spec, expr_t const& expr) const {
	Pr printer{&out};
	bool const version = !spec.main.version.empty();
	typename spec_t::opt_t help;
	help.id = printer.help_id();
	help.description = "Show help text";
	spec.main.options.push_back(help);
	if (version) {
		typename spec_t::opt_t version;
		version.id = printer.version_id();
		version.description = "Show version";
		spec.main.options.push_back(version);
	}
	fill_opts(spec.main.options);
	if (option_match(printer.help_id(), false, expr.command.id)) {
		printer(spec);
		return result::quit;
	}
	for (auto const& [id, _] : expr.options) {
		if (option_match(printer.help_id(), true, id)) {
			printer(spec);
			return result::quit;
		} else if (version && option_match(printer.version_id(), false, id)) {
			printer(spec.main.version);
			return result::quit;
		}
	}
	if (auto const it = spec.commands.find(expr.command.id); it != spec.commands.end()) {
		auto const& cmd = it->second;
		for (auto const& opt : expr.command.options) {
			if (option_match(printer.help_id(), true, opt.id)) {
				fill_opts(cmd.options);
				printer(spec.main.exe, it->first, cmd);
				return result::quit;
			}
		}
	}
	return result::resume;
}

template <typename Ch>
void basic_helper<Ch>::fill_opts(std::vector<typename spec_t::opt_t> const& out_options) {
	std::unordered_map<char_t, int> firsts;
	for (auto const& opt : out_options) {
		if (!opt.id.empty() && opt.id[0] != null_char) {
			++firsts[opt.id[0]];
		}
	}
	for (auto const& opt : out_options) {
		if (!opt.id.empty() && firsts[opt.id[0]] == 1) {
			opt.single = opt.id[0];
		}
	}
}

template <typename Ch>
bool basic_helper<Ch>::option_match(string_view_t id, bool single, string_view_t str) noexcept {
	return !id.empty() && (str == id || (single && str.size() == 1 && str[0] == id[0]));
}
} // namespace clap
