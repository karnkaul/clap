#include <cassert>
#include <iostream>
#include <clap/clap.hpp>
#include <clap_version.hpp>

namespace clap {
str_t unquote(str_t str) noexcept {
	auto const palindrome = [](str_t str, char c) { return str[0] == c && str.back() == c; };
	if (str.size() > 1 && (palindrome(str, '\"') || palindrome(str, '\''))) { return str.substr(1, str.size() - 2); }
	return str;
}

struct state_impl {
	str_t cmd_;
	parse_info info;
	option_parser* parser{};
	int argc{};
	char const* const* argv{};
	int next{};
	bool quit{};
	int opts_parsed{};
	int cmds_parsed{};
	int arg_idx{};

	operator parse_state() noexcept { return parse_state(this); }
};

parse_info const& parse_state::info() const noexcept { return m_impl->info; }
option_parser const& parse_state::parser() const noexcept {
	assert(m_impl->parser);
	return *m_impl->parser;
}
int parse_state::opts_parsed() const noexcept { return m_impl->opts_parsed; }
int parse_state::cmds_parsed() const noexcept { return m_impl->cmds_parsed; }
int parse_state::arg_index() const noexcept { return m_impl->arg_idx; }
str_t parse_state::cmd() const noexcept { return m_impl->cmd_; }
bool parse_state::quit() const noexcept { return m_impl->quit; }
void parse_state::early_exit(bool set_quit) noexcept {
	m_impl->next = m_impl->argc;
	if (set_quit) { m_impl->quit = true; }
}

void version(parse_info const& info) { (*info.cout) << info.root->program->version << '\n'; }
void usage_full(parse_info const& info, str_t cmd_key, option::flags_t root_mask);
void help(parse_info const& info, str_t cmd_key);

struct root_parser_impl : root_parser {
	enum key { k_version = -1, k_usage = -2, ehelp = -3 };
	enum flag_t { flag_force = 1 << 16 };

	root_parser_impl() {
		static constexpr option opts[] = {
			{ehelp, "help", "Show this help text", {}, flag_force},
			{k_version, "version", "Show version"},
			{k_usage, "usage", "Show usage", {}, flag_force},
		};
		spec.options = opts;
	}

	bool operator()(option_key key, str_t, parse_state state) override {
		auto ret = [](parse_state st, auto f) {
			if (st.opts_parsed() == 0) {
				f();
				st.early_exit(true);
				return true;
			}
			return false;
		};
		switch (key) {
		case ehelp: {
			help(state.info(), state.cmd());
			state.early_exit(true);
			return true;
		}
		case k_version: return ret(state, [state]() { version(state.info()); });
		case k_usage: return ret(state, [state]() { usage_full(state.info(), state.cmd(), state.cmd().empty() ? 0 : flag_force); });
		case no_end: return true;
		default: return false;
		}
	}
};

class args_parser {
  public:
	parse_result operator()(program_spec const& spec, int argc, char const* const argv[], parse_info info);

  private:
	bool parse();

	bool option(str_t opt, str_t arg, option::flags_t mask, bool check_parser = true);
	bool option(char key);
	bool option_peek(char key);
	bool option_consume(char key, str_t arg, bool* consumed);

	str_t peek() const noexcept;
	str_t next() noexcept;

	state_impl m_state;
};

constexpr bool test(option::flags_t flags, option::flag_t flag) noexcept { return (flags & flag) == flag; }
constexpr bool pass(option::flags_t flags, option::flags_t mask) noexcept { return (mask == 0) || (flags & mask) == mask; }
constexpr bool optional(option::flags_t flags) noexcept { return test(flags, option::flag_optional); }
constexpr bool alpha(char c) noexcept { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }

str_t exe_name(char const* arg0) noexcept {
	str_t ret = arg0;
	auto i = ret.find_last_of('/');
	if (i >= ret.size()) { i = ret.find_last_of('\\'); }
	if (i < ret.size()) { return ret.substr(i + 1); }
	return ret;
}

std::ostream& opt_key_arg(std::ostream& out, option const& op) {
	if (op.key.is_alpha()) {
		out << " [-" << op.key.alpha();
		if (!op.arg.empty()) {
			if ((op.flags & option::flag_optional) == option::flag_optional) {
				out << '[' << op.arg << ']';
			} else {
				out << op.arg;
			}
		}
		out << "]";
	}
	return out;
}

std::ostream& opt_str_arg(std::ostream& out, option const& op) {
	out << " [--" << op.name;
	if (!op.arg.empty()) {
		if ((op.flags & option::flag_optional) == option::flag_optional) {
			out << "[=" << op.arg << ']';
		} else {
			out << '=' << op.arg;
		}
	}
	out << "]";
	return out;
}

option_parser* get_cmd(root_parser const& root, str_t key) noexcept {
	if (!key.empty()) {
		for (option_parser* p : root.program->cmds) {
			if (p && p->spec.cmd_key == key) { return p; }
		}
	}
	return nullptr;
}

std::size_t longest_name(root_parser const& root, option_parser const* cmd) noexcept {
	std::size_t ret{};
	auto check = [&ret](option_parser const* parser) {
		for (auto const& o : parser->spec.options) {
			std::size_t total = o.name.size();
			if (!o.arg.empty()) {
				total += o.arg.size();
				if (optional(o.flags)) { total += 2; }
			}
			ret = std::max(ret, total);
		}
	};
	check(&root);
	if (cmd) {
		check(cmd);
	} else {
		check(root.parser());
	}
	for (option_parser const* cmd : root.program->cmds) { ret = std::max(ret, cmd->spec.cmd_key.size()); }
	return ret;
}

std::ostream& usage_short(std::ostream& out, root_parser const& root, str_t cmd_key) {
	out << "Usage: " << root.program->name;
	auto parser_usage = [](std::ostream& out, option_parser const& parser) {
		out << " [OPTION...]";
		if (!parser.spec.arg_doc.empty()) { out << ' ' << parser.spec.arg_doc; }
	};
	if (auto cmd = get_cmd(root, cmd_key)) {
		out << ' ' << cmd->spec.cmd_key;
		parser_usage(out, *cmd);
	} else {
		if (!root.program->cmds.empty()) {
			if (root.program->cmds_required > 0) {
				out << " CMD";
			} else {
				out << " [CMD]";
			}
		}
		parser_usage(out, *root.parser());
	}
	return out;
}

void usage_full(parse_info const& info, str_t cmd_key, option::flags_t root_mask) {
	(*info.cout) << "Usage: " << info.root->program->name;
	auto cmd = get_cmd(*info.root, cmd_key);
	if (cmd) {
		(*info.cout) << ' ' << cmd->spec.cmd_key;
	} else if (!info.root->program->cmds.empty()) {
		if (info.root->program->cmds_required > 0) {
			(*info.cout) << " CMD";
		} else {
			(*info.cout) << " [CMD]";
		}
	}
	auto parser = cmd ? cmd : info.root->parser();
	auto parser_opts = [cout = info.cout](option_parser const& parser, option::flags_t root_mask) {
		for (auto const& o : parser.spec.options) { opt_key_arg(*cout, o); }
		for (auto const& o : parser.spec.options) {
			if (pass(o.flags, root_mask)) { opt_str_arg(*cout, o); }
		}
	};
	parser_opts(*parser, root_mask);
	parser_opts(*info.root, root_mask);
	if (!parser->spec.arg_doc.empty()) { (*info.cout) << ' ' << parser->spec.arg_doc; }
	(*info.cout) << '\n';
}

struct pad_printer {
	std::ostream& out;
	int width{};
	int inserted{};

	pad_printer(std::ostream& out) : out(out) {}

	pad_printer& align() {
		for (int remain = width - inserted; remain > 0; --remain) { out << ' '; }
		inserted = width;
		return *this;
	}

	void reset() { inserted = width = 0; }

	friend pad_printer& operator<<(pad_printer& pr, char c) {
		if (pr.width > 0) { ++pr.inserted; }
		pr.out << c;
		return pr;
	}

	friend pad_printer& operator<<(pad_printer& pr, str_t str) {
		if (pr.width > 0) { pr.inserted += static_cast<int>(str.size()); }
		pr.out << str;
		return pr;
	}
};

void print_option(parse_info const& info, option const& opt, int width) {
	pad_printer pr{*info.cout};
	pr << "  ";
	pr.width = 4;
	if (opt.key.is_alpha()) { pr << '-' << opt.key.alpha() << ", "; }
	pr.align() << "--";
	pr.reset();
	pr.width = width;
	if (!opt.arg.empty()) {
		if (optional(opt.flags)) {
			pr << opt.name << "[=" << opt.arg << "]";
		} else {
			pr << opt.name << '=' << opt.arg;
		}
	} else {
		pr << opt.name;
	}
	pr.align();
	pr << "\t\t" << opt.doc << '\n';
}

void print_cmd(parse_info const& info, option_parser const& cmd, int width) {
	pad_printer pr{*info.cout};
	pr.width = 8;
	pr.align();
	pr.reset();
	pr.width = width;
	pr << cmd.spec.cmd_key;
	pr.align();
	pr << "\t\t" << cmd.spec.cmd_doc << '\n';
}

void options_full(parse_info const& info, str_t cmd_key, int width, option::flags_t root_mask) {
	auto cmd = get_cmd(*info.root, cmd_key);
	auto parser = cmd ? cmd : info.root->parser();
	bool const has_options = !parser->spec.options.empty() || !info.root->spec.options.empty();
	if (has_options) {
		(*info.cout) << "OPTIONS\n";
		for (auto const& o : parser->spec.options) { print_option(info, o, width); }
		for (auto const& o : info.root->spec.options) {
			if (pass(o.flags, root_mask)) { print_option(info, o, width); }
		}
		(*info.cout) << '\n';
	}
}

void cmds_full(parse_info const& info, int width) {
	if (!info.root->program->cmds.empty()) {
		(*info.cout) << "CMDS\n";
		for (option_parser const* cmd : info.root->program->cmds) {
			if (cmd) { print_cmd(info, *cmd, width); }
		}
		(*info.cout) << '\n';
	}
}

void help(parse_info const& info, str_t cmd_key) {
	usage_short(*info.cout, *info.root, cmd_key) << '\n';
	auto cmd = get_cmd(*info.root, cmd_key);
	if (cmd) {
		if (!cmd->spec.cmd_doc.empty()) { (*info.cout) << '\n' << cmd->spec.cmd_doc << '\n'; }
	} else if (!info.root->program->doc.empty()) {
		(*info.cout) << info.root->program->name << " -- " << info.root->program->doc << '\n';
	}
	(*info.cout) << '\n';
	auto const width = static_cast<int>(longest_name(*info.root, cmd));
	if (!cmd) { cmds_full(info, width); }
	options_full(info, cmd_key, width, cmd ? root_parser_impl::flag_force : 0);
	option_parser const* p = cmd ? cmd : info.root->parser();
	if (!p->spec.doc_desc.empty()) { (*info.cout) << p->spec.doc_desc << '\n'; }
}

std::ostream& print_try(std::ostream& out, str_t program_name, str_t cmd_key, bool usage) {
	out << "Try '" << program_name;
	if (!cmd_key.empty()) { out << ' ' << cmd_key; }
	out << " --help'";
	if (usage) {
		out << " or '" << program_name;
		if (!cmd_key.empty()) { out << ' ' << cmd_key; }
		out << " --usage'";
	}
	return out << " for more information.";
}

void option_parser::usage(parse_state state) {
	usage_short(*state.info().cout, *state.m_impl->info.root, state.cmd()) << '\n';
	print_try(*state.info().cout, state.m_impl->info.root->program->name, {}, true) << '\n';
}

std::ostream& print_option_id(std::ostream& out, option const& opt) {
	if (opt.key.is_alpha()) {
		return out << '\'' << opt.key.alpha() << '\'';
	} else {
		return out << '\'' << opt.name << '\'';
	}
}

bool err_missing_cmd(parse_info const& info) {
	(*info.cerr) << info.root->program->name << ": requires a cmd\n";
	print_try((*info.cerr), info.root->program->name, {}, false) << '\n';
	return false;
}

bool err_unknown_cmd(parse_info const& info, str_t cmd) {
	(*info.cerr) << "Unrecognized cmd -- '" << cmd << "'\n";
	print_try((*info.cerr), info.root->program->name, {}, false) << '\n';
	return false;
}

bool err_unknown_arg(parse_info const& info, str_t cmd, str_t arg) {
	(*info.cerr) << "Unrecognized argument -- '" << arg << "'\n";
	print_try((*info.cerr), info.root->program->name, cmd, false) << '\n';
	return false;
}

bool err_missing_arg(parse_info const& info, str_t v, option const& opt) {
	(*info.cerr) << info.root->program->name << ": option requires an argument -- ";
	print_option_id((*info.cerr), opt) << '\n';
	print_try((*info.cerr), info.root->program->name, v, false) << '\n';
	return false;
}

bool err_opt_arg(parse_info const& info, str_t cmd_key, option const& opt, str_t arg) {
	(*info.cerr) << info.root->program->name << ":";
	if (!arg.empty() && opt.arg.empty()) {
		(*info.cerr) << " option does not take an argument -- ";
	} else {
		(*info.cerr) << " invalid argument to option -- ";
	}
	print_option_id((*info.cerr), opt) << '\n';
	print_try((*info.cerr), info.root->program->name, cmd_key, false) << '\n';
	return false;
}

bool err_unknown_opt(parse_info const& info, str_t cmd_key, int key, str_t name) {
	(*info.cerr) << "Unrecognized option -- ";
	print_option_id((*info.cerr), {key, name}) << '\n';
	print_try((*info.cerr), info.root->program->name, cmd_key, false) << '\n';
	return false;
}

bool parse_option(parse_info const& info, option_parser& parser, option const& opt, str_t arg, parse_state st, option::flags_t mask) {
	if (opt.arg.empty() && !arg.empty()) {
		return err_opt_arg(info, st.cmd(), opt, arg);
	} else if (mask != 0) {
		if (pass(opt.flags, mask) && parser(opt.key, arg, st)) { return true; }
		return false;
	} else if (parser(opt.key, arg, st)) {
		return true;
	} else {
		return err_opt_arg(info, st.cmd(), opt, arg);
	}
}

parse_result args_parser::operator()(program_spec const& spec, int argc, char const* const argv[], parse_info info) {
	root_parser_impl r;
	if (!info.root) { info.root = &r; }
	if (!info.cout) { info.cout = &std::cout; }
	if (!info.cerr) { info.cerr = &std::cerr; }
	info.root->program = &spec;
	if (info.root->program->name.empty() && argc > 0) { info.root->program->name = exe_name(argv[0]); }
	m_state = state_impl{{}, info, info.root->parser(), argc, argv, 1};
	if (parse()) { return m_state.quit ? parse_result::quit : parse_result::run; }
	return parse_result::parse_error;
}

str_t args_parser::peek() const noexcept {
	if (m_state.next < m_state.argc) { return m_state.argv[m_state.next]; }
	return {};
}

str_t args_parser::next() noexcept {
	auto ret = peek();
	++m_state.next;
	return ret;
}

bool args_parser::parse() {
	bool no_more_opts{};
	while (!peek().empty()) {
		str_t arg = next();
		if (arg[0] == '-' && !no_more_opts) {
			// option
			option::flags_t const mask = m_state.cmd_.empty() ? 0 : root_parser_impl::flag_force;
			if (arg = arg.substr(1); !arg.empty()) {
				if (arg[0] == '-') {
					// --, --abcd, --ab=cd
					str_t opt = arg.substr(1);
					if (opt.empty()) {
						// --
						no_more_opts = true;
						// consume this arg
						continue;
					} else if (auto i = opt.find('='); i < opt.size()) {
						// --ab=cd
						arg = unquote(opt.substr(i + 1));
						opt = opt.substr(0, i);
					} else {
						// --abcd
						arg = {};
					}
					if (!option(opt, arg, mask)) { return false; }
				} else {
					// -a bcd, -abcd, -a=bcd
					if (arg.size() == 1) {
						// -a bcd
						if (!option_peek(arg[0])) { return false; }
					} else {
						if (arg[1] == '=') {
							// a=bcd
							str_t argv = arg.substr(2);
							if (argv.empty() || !option(arg.substr(0, 1), unquote(argv), mask)) { return false; }
						} else {
							// disambiguate -a bcd vs -a -b -c -d
							str_t argv = arg.substr(1);
							bool consumed{};
							if (!option_consume(arg[0], unquote(argv), &consumed)) { return false; }
							if (!consumed) {
								// a,b,c,d
								for (char const opt : argv) {
									++m_state.opts_parsed; // pre-increment to account for already parsed 'a'
									if (!option(opt)) { return false; }
								}
							}
						}
					}
				}
			}
			++m_state.opts_parsed;
		} else {
			// check for cmd
			if (auto cmd = get_cmd(*m_state.info.root, arg)) {
				m_state.cmd_ = cmd->spec.cmd_key;
				m_state.parser = cmd;
				++m_state.cmds_parsed;
				// consume this arg
				continue;
			}
			// non-option
			if (!(*m_state.parser)(option_parser::no_arg, arg, m_state)) { return err_unknown_arg(m_state.info, m_state.cmd_, arg); }
			++m_state.arg_idx;
		}
	}
	if (!m_state.quit) {
		if (m_state.cmds_parsed < m_state.info.root->program->cmds_required) { return err_missing_cmd(m_state.info); }
		// end
		if (!(*m_state.parser)(option_parser::no_end, {}, m_state)) {
			m_state.parser->usage(m_state);
			return false;
		}
	}
	return true;
}

bool args_parser::option(str_t opt, str_t arg, option::flags_t mask, bool check_parser) {
	auto check = [this, arg, opt](option_parser* parser, option::flags_t mask, bool* unknown_opt) {
		*unknown_opt = false;
		for (auto const& o : parser->spec.options) {
			if ((opt.size() == 1 && opt[0] == o.key) || o.name == opt) {
				if (!o.arg.empty() && arg.empty() && !optional(o.flags)) { return err_missing_arg(m_state.info, m_state.cmd_, o); }
				return parse_option(m_state.info, *parser, o, arg, m_state, mask);
			}
		}
		*unknown_opt = true;
		return false;
	};
	bool unknown_opt{};
	if (check(m_state.info.root, mask, &unknown_opt) || !check_parser || check(m_state.parser, 0, &unknown_opt)) { return true; }
	return unknown_opt ? err_unknown_opt(m_state.info, m_state.cmd_, 0, opt) : false;
}

bool args_parser::option(char key) {
	for (auto const& o : m_state.parser->spec.options) {
		if (o.key != char{} && o.key == key) {
			if (!o.arg.empty() && !optional(o.flags)) { return err_missing_arg(m_state.info, m_state.cmd_, o); }
			return parse_option(m_state.info, *m_state.parser, o, {}, m_state, 0);
		}
	}
	return err_unknown_opt(m_state.info, m_state.cmd_, key, {});
}

bool args_parser::option_peek(char key) {
	for (auto const& o : m_state.parser->spec.options) {
		if (o.key != char{} && o.key == key) {
			if (!o.arg.empty() && !optional(o.flags)) {
				if (peek().empty()) { return err_missing_arg(m_state.info, m_state.cmd_, o); }
				return parse_option(m_state.info, *m_state.parser, o, next(), m_state, 0);
			} else {
				return parse_option(m_state.info, *m_state.parser, o, {}, m_state, 0);
			}
		}
	}
	return err_unknown_opt(m_state.info, m_state.cmd_, key, {});
}

bool args_parser::option_consume(char key, str_t arg, bool* consumed) {
	for (auto const& o : m_state.parser->spec.options) {
		if (o.key != char{} && o.key == key) {
			*consumed = !o.arg.empty();
			return parse_option(m_state.info, *m_state.parser, o, *consumed ? arg : str_t{}, m_state, 0);
		}
	}
	return err_unknown_opt(m_state.info, m_state.cmd_, key, {});
}

parse_result parse_args(program_spec const& cmd, int argc, char const* const argv[], parse_info const& info) { return args_parser{}(cmd, argc, argv, info); }

str_t lib_version() noexcept { return clap_version; }
} // namespace clap
