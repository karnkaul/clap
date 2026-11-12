module;

#include <cassert>

module clap.detail;

namespace clap::detail {
namespace {
[[nodiscard]] constexpr auto strip_quotes(std::string_view in) {
	if (in.starts_with('"')) {
		in.remove_prefix(1);
		if (in.ends_with('"')) {
			in.remove_suffix(1);
		}
	}
	return in;
}
} // namespace

Scanner::Scanner(std::span<std::string_view const> words) : m_remain(words) { advance(); }

auto Scanner::scan_next(Token& out) -> bool {
	if (m_current.empty()) {
		out = Token{.type = Token::Type::Eof};
		return false;
	}

	out = scan_next();
	return true;
}

auto Scanner::scan_next() -> Token {
	if (m_current.starts_with('"')) {
		return scan_quoted();
	}

	if (m_current == "--") {
		return to_token(Token::Type::MinusMinus, 2);
	}

	if (m_current.starts_with("--")) {
		return to_token(Token::Type::MinusMinusString);
	}
	if (m_current.starts_with('-')) {
		return to_token(Token::Type::MinusString);
	}

	return to_token(Token::Type::String);
}

auto Scanner::scan_quoted() -> Token {
	m_current = strip_quotes(m_current);
	return to_token(Token::Type::String);
}

void Scanner::advance() {
	if (m_remain.empty()) {
		m_current = {};
		return;
	}
	m_current = m_remain.front();
	m_remain = m_remain.subspan(1);
}

auto Scanner::to_token(Token::Type const type, std::size_t length) -> Token {
	if (length == 0) {
		length = m_current.size();
	}
	auto const ret = Token{.type = type, .lexeme = m_current.substr(0, length)};
	m_current.remove_prefix(length);
	if (m_current.empty()) {
		advance();
	}
	return ret;
}

PrinterWrapper::PrinterWrapper(Ptr<IPrinter> printer, std::string_view program_name) noexcept(true)
	: printer(printer ? printer : &IPrinter::default_printer()),
	  program_name(program_name) {}

Frame::Frame(std::span<Parameter const> parameters) noexcept(false) {
	named_parameters.reserve(parameters.size());
	positional_parameters.reserve(parameters.size());

	auto positionals_ended = false;
	auto const check_extraneous = [&](auto const& t) {
		if (!positionals_ended) {
			return;
		}
		throw InvalidParameterException{std::format("extraneous positional: '{}'", t.name)};
	};
	auto required_ended = false;
	auto const check_required_after_optional = [&](parameter::Positional const& p) {
		if (p.type == parameter::Type::Optional) {
			required_ended = true;
			return;
		}
		if (!required_ended) {
			return;
		}
		throw InvalidParameterException{std::format("required positional after optional(s): '{}'", p.name)};
	};

	auto const visitor = Visitor{
		[&](parameter::Named const& n) {
			named_parameters.push_back(&n);
		},
		[&](parameter::Positional const& p) {
			check_extraneous(p);
			check_required_after_optional(p);
			positional_parameters.push_back(&p);
		},
		[&](parameter::List const& l) {
			check_extraneous(l);
			list_parameter = &l;
			positionals_ended = true;
		},
	};

	for (auto const& parameter : parameters) {
		std::visit(visitor, parameter);
	}
}

Frame::Frame(std::span<parameter::Named const> options) noexcept(true) {
	named_parameters.reserve(options.size());
	for (auto const& parameter : options) {
		named_parameters.push_back(&parameter);
	}
}

Context::Context(spec::Parameters spec) noexcept(false)
	: m_parameters(std::move(spec.parameters)),
	  printer(spec.custom_printer, spec.program.name),
	  main(m_parameters),
	  version(spec.program.version),
	  description(spec.program.description) {}

Context::Context(spec::Commands spec) noexcept(false)
	: m_options(std::move(spec.options)),
	  m_commands(std::move(spec.commands)),
	  printer(spec.custom_printer, spec.program.name),
	  main(m_options),
	  version(spec.program.version),
	  description(spec.program.description),
	  command_policy(spec.policy) {
	commands.reserve(m_commands.size());
	for (auto const& command : m_commands) {
		auto frame = Frame{command.parameters};
		frame.command = &command;
		commands.push_back(std::move(frame));
	}
}

void Context::override_program_name_if_empty(std::string_view const program_name) {
	if (!printer.program_name.empty()) {
		return;
	}
	printer.program_name = program_name;
}
} // namespace clap::detail

namespace clap::detail {
namespace {
struct ArgSplit {
	[[nodiscard]] static constexpr auto create(std::string_view const word) {
		auto ret = ArgSplit{.left = strip_quotes(word)};
		if (auto const i = ret.left.find('='); i != std::string_view::npos) {
			ret.right = ret.left.substr(i + 1);
			ret.left = ret.left.substr(0, i);
		}
		ret.right = strip_quotes(ret.right);
		return ret;
	}

	std::string_view left{};
	std::string_view right{};
};

struct Joiner {
	void join(std::string_view const s) { join("{}", s); }

	template <typename... Args>
	void join(std::format_string<Args...> fmt, Args&&... args) {
		if (!text.empty() && text.back() != '\n' && !text.ends_with(delimiter)) {
			text.append(delimiter);
		}
		std::format_to(std::back_inserter(text), fmt, std::forward<Args>(args)...);
	}

	std::string_view delimiter{" "};

	std::string text{};
};

void serialize_positionals_usage(Joiner& joiner,
								 std::span<Ptr<parameter::Positional const> const> positionals,
								 Ptr<parameter::List const> list) {
	for (auto const& positional : positionals) {
		if (positional->type == parameter::Type::Optional) {
			joiner.join("[{}]", positional->name);
		} else {
			joiner.join("<{}>", positional->name);
		}
	}
	if (list) {
		joiner.join("[{}]...", list->name);
	}
	joiner.text.append("\n");
}

void serialize_named_list(Joiner& out,
						  std::span<Ptr<parameter::Named const> const> parameters,
						  bool const add_version) {
	out.text.append("\nOPTIONS\n");
	for (auto const* named : parameters) {
		out.text.append("  ");
		if (named->letter) {
			std::format_to(std::back_inserter(out.text), "-{}", named->letter);
			if (!named->word.empty()) {
				out.text.append(", ");
			}
		} else {
			out.text.append("    ");
		}
		if (!named->word.empty()) {
			std::format_to(std::back_inserter(out.text), "--{}", named->word);
		}
		out.text.append("\n");
		if (!named->description.empty()) {
			out.join("{: >8}{}\n", ' ', named->description);
		}
	}
	if (add_version) {
		std::format_to(std::back_inserter(out.text), "{: >6}--version\n{: >8}display version and exit\n", ' ', ' ');
	}
	std::format_to(std::back_inserter(out.text), "{: >6}--help\n{: >8}display this help and exit\n", ' ', ' ');
}

void serialize_positional_list(Joiner& out,
							   std::span<Ptr<parameter::Positional const> const> parameters,
							   Ptr<parameter::List const> list) {
	if (parameters.empty() && !list) {
		return;
	}
	out.text.append("\nARGUMENTS\n");
	for (auto const* positional : parameters) {
		out.join("  {}\n", positional->name);
		if (!positional->description.empty()) {
			out.join("{: >8}{}\n", ' ', positional->description);
		}
	}
	if (list) {
		out.join("  {}\n", list->name);
		if (!list->description.empty()) {
			out.join("{: >8}{}\n", ' ', list->description);
		}
	}
}

void serialize_command_list(Joiner& out, std::span<Frame const> commands) {
	if (commands.empty()) {
		return;
	}
	out.text.append("\nCOMMANDS\n");
	for (auto const& frame : commands) {
		out.join("  {}\n", frame.command->identifier);
		if (!frame.command->description.empty()) {
			out.join("{: >8}{}\n", ' ', frame.command->description);
		}
	}
}
} // namespace

ParserImpl::Environment::Environment(Context const& context) noexcept(false)
	: printer(context.printer),
	  commands(context.commands),
	  version(context.version),
	  description(context.description),
	  command_policy(context.command_policy),
	  frame(&context.main) {}

auto ParserImpl::Environment::next_positional() -> Ptr<parameter::Positional const> {
	if (positional_index >= frame->positional_parameters.size()) {
		return nullptr;
	}
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	return frame->positional_parameters[positional_index++];
}

auto ParserImpl::Environment::set_command_frame(std::string_view const identifier) -> bool {
	auto const it = std::ranges::find_if(commands, [identifier](Frame const& f) {
		return f.command->identifier == identifier;
	});
	if (it == commands.end()) {
		return false;
	}

	frame = &*it;
	positional_index = 0uz;
	return true;
}

auto ParserImpl::Environment::help_text_parameters(bool const for_command) const -> std::string {
	assert(!for_command || frame->command); // NOLINT(misc-static-assert)

	auto joiner = Joiner{};

	joiner.text = "Usage:";
	joiner.join(printer.program_name);
	if (for_command) {
		joiner.join("{}", frame->command->identifier);
	}
	joiner.join("[OPTION]...");
	serialize_positionals_usage(joiner, frame->positional_parameters, frame->list_parameter);

	auto const desc = for_command ? frame->command->description : description;
	if (!desc.empty()) {
		joiner.join("{}\n", desc);
	}

	auto const add_version = !for_command && !version.empty();
	serialize_named_list(joiner, frame->named_parameters, add_version);
	serialize_positional_list(joiner, frame->positional_parameters, frame->list_parameter);

	return std::move(joiner.text);
}

auto ParserImpl::Environment::help_text_commands() const -> std::string {
	auto joiner = Joiner{};

	joiner.text = "Usage:";
	joiner.join(printer.program_name);
	joiner.join("[OPTION]...");
	joiner.join(command_policy == CommandPolicy::Optional ? "[COMMAND]" : "<COMMAND>");
	joiner.join("[OPTION]... <ARG>...\n");

	if (!description.empty()) {
		joiner.join("{}\n", description);
	}

	serialize_named_list(joiner, frame->named_parameters, !version.empty());
	serialize_command_list(joiner, commands);

	return std::move(joiner.text);
}

auto ParserImpl::Environment::help_text() const -> std::string {
	if (frame->command) {
		return help_text_parameters(true);
	}
	if (!commands.empty()) {
		return help_text_commands();
	}
	return help_text_parameters(false);
}

auto ParserImpl::Environment::is_missing_required_command() const -> bool {
	return !commands.empty() && !frame->command && command_policy == CommandPolicy::Required;
}

ParserImpl::ParserImpl(Context const& context, std::span<std::string_view const> args) noexcept(true)
	: m_environment(context),
	  m_scanner(args) {}

auto ParserImpl::parse() -> Result {
	advance();
	while (m_current.type != Token::Type::Eof && m_outcome == Outcome::Continue) {
		parse_current();
	}
	auto ret = Result{.outcome = m_outcome};
	if (m_environment.frame->command) {
		ret.command_identifier = m_environment.frame->command->identifier;
	}
	if (m_outcome != Outcome::EarlyExit) {
		check_required_parsed();
		if (m_environment.is_missing_required_command()) {
			m_environment.printer.prefixed_err("missing required command");
			throw Error{Error::MissingRequiredCommand};
		}
	}
	return ret;
}

void ParserImpl::advance() { m_scanner.scan_next(m_current); }

void ParserImpl::parse_current() {
	if (m_options_terminated) {
		parse_argument();
		return;
	}

	switch (m_current.type) {
	case Token::Type::MinusMinus:
		m_options_terminated = true;
		advance();
		break;
	case Token::Type::String: parse_argument(); break;
	case Token::Type::MinusMinusString: parse_long_option(); break;
	case Token::Type::MinusString: parse_short_options(); break;

	case Token::Type::Eof:
		m_environment.printer.prefixed_err("unexpected token: '{}'", m_current.lexeme);
		throw Error{Error::UnexpectedToken};

	default:
		m_environment.printer.prefixed_err("unrecognized token: '{}'", m_current.lexeme);
		throw Error{Error::UnrecognizedToken};
	}
}

void ParserImpl::parse_argument() {
	if (select_command()) {
		return;
	}

	auto const* positional = m_environment.next_positional();
	if (!positional) {
		if (!m_environment.frame->list_parameter) {
			m_environment.printer.prefixed_err("unknown argument: '{}'", m_current.lexeme);
			throw Error{Error::UnknownArgument};
		}
		if (!m_environment.frame->list_parameter->parse(m_current.lexeme)) {
			m_environment.printer.prefixed_err(
				"invalid {}: '{}'", m_environment.frame->list_parameter->name, m_current.lexeme);
			throw Error{Error::InvalidArgument};
		}
		advance();
		return;
	}

	if (!positional->parse(m_current.lexeme)) {
		m_environment.printer.prefixed_err("invalid {}: '{}'", positional->name, m_current.lexeme);
		throw Error{Error::InvalidArgument};
	}
	advance();
}

void ParserImpl::parse_long_option() {
	assert(m_current.lexeme.starts_with("--") && m_current.lexeme.size() > 2); // NOLINT(misc-static-assert)

	auto const option_token = m_current;
	auto const word = option_token.lexeme.substr(2);

	if (!m_environment.frame->command && !m_environment.version.empty() && word == "version") {
		m_environment.printer->println(m_environment.version);
		m_outcome = Outcome::EarlyExit;
		return;
	}

	if (word == "help") {
		m_environment.printer->println(help_text());
		m_outcome = Outcome::EarlyExit;
		return;
	}

	auto const split = ArgSplit::create(word);
	auto const& named = get_named(split.left);
	parse_option_value(named, split.right, option_token.lexeme);
}

void ParserImpl::parse_short_options() {
	assert(m_current.lexeme.front() == '-' && m_current.lexeme.size() > 1); // NOLINT(misc-static-assert)

	auto const option_token = m_current;

	auto const word = option_token.lexeme.substr(1);
	auto const split = ArgSplit::create(word);
	auto letters = split.left;
	for (; letters.size() > 1; letters.remove_prefix(1)) {
		auto const& named = get_named(letters.front());
		if (!named.is_flag) {
			m_environment.printer.prefixed_err("option requires an argument -- '{}'", named.letter);
			throw Error{Error::OptionRequiresArgument};
		}
		named.parse("true");
	}

	auto const& named = get_named(letters.front());
	parse_option_value(named, split.right, option_token.lexeme);
}

void ParserImpl::parse_option_value(parameter::Named const& named,
									std::string_view value,
									std::string_view const option_lexeme) {
	if (!named.is_flag && value.empty()) {
		advance();
		value = m_current.lexeme;
	}

	if (!value.empty()) {
		if (!named.parse(value)) {
			m_environment.printer.prefixed_err("invalid '{}': {}", named.word, value);
			throw Error{Error::InvalidArgument};
		}
		advance();
		return;
	}

	if (!named.is_flag) {
		m_environment.printer.prefixed_err("option '{}' requires an argument", option_lexeme);
		throw Error{Error::OptionRequiresArgument};
	}

	named.parse("true");
	advance();
}

template <typename T>
auto ParserImpl::get_named(T const t) const -> parameter::Named const& {
	auto const pred = [t](Ptr<parameter::Named const> n) {
		if constexpr (std::same_as<T, char>) {
			return n->letter == t;
		} else {
			return n->word == t;
		}
	};
	auto const it = std::ranges::find_if(m_environment.frame->named_parameters, pred);
	if (it != m_environment.frame->named_parameters.end()) {
		return **it;
	}
	m_environment.printer.prefixed_err("unrecognized option: '{}'", m_current.lexeme);
	throw Error{Error::UnrecognizedOption};
}

auto ParserImpl::select_command() -> bool {
	if (m_environment.frame->command) {
		return false;
	}
	auto const ret = m_environment.set_command_frame(m_current.lexeme);
	if (ret) {
		advance();
	}
	return ret;
}

void ParserImpl::check_required_parsed() {
	auto const* remaining = m_environment.next_positional();
	if (!remaining || remaining->type == parameter::Type::Optional) {
		return;
	}
	m_environment.printer.prefixed_err("missing required argument: '{}'", remaining->name);
	throw Error{Error::MissingRequiredArgument};
}
} // namespace clap::detail
