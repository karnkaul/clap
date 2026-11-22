module clap.parser;

namespace clap {
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

// void prefixed_err(IPrinter& printer, std::string_view const program_name, std::string_view const msg) {
// 	auto message = std::string{program_name};
// 	if (!message.empty()) {
// 		message += ": ";
// 	}
// 	std::format_to(std::back_inserter(message), "{}", msg);
// 	printer.printerr(message);
// }

[[nodiscard]] auto or_iterate(Result const result, OptionParser::Iterate const it) -> OptionParser::Action {
	if (result.should_early_exit()) {
		return result;
	}
	return it;
}
} // namespace

OptionParser::OptionParser(IContext& context, std::span<parameter::Named const> options)
	: m_context(context),
	  m_options(options) {}

auto OptionParser::parse_next(Token const& token) -> Action {
	if (m_force_ended) {
		return Iterate::Pass;
	}

	if (m_expects_value) {
		if (token.type == Token::Type::String) {
			auto const ret = assign_value_to(*m_expects_value, token.lexeme, m_option_lexeme);
			m_expects_value = {};
			m_option_lexeme = {};
			return or_iterate(ret, Iterate::Consume);
		}

		m_context.print_error(std::format("option '{}' requires an argument", m_option_lexeme));
		return Result::create_early_exit(ExitCode::OptionRequiresArgument);
	}

	switch (token.type) {
	case Token::Type::String: return Iterate::Pass;
	case Token::Type::MinusMinus: m_force_ended = true; return Iterate::Consume;
	case Token::Type::MinusMinusString: return parse_long(token);
	case Token::Type::MinusString: return parse_shorts(token);
	default: break;
	}

	m_context.print_error(std::format("InternalError: unexpected Token: {}", token.lexeme));
	return Result::create_early_exit(ExitCode::InternalError);
}

auto OptionParser::finish() -> Result {
	if (!m_expects_value) {
		return Result::create_continue();
	}
	m_context.print_error(std::format("option '{}' requires an argument", m_option_lexeme));
	return Result::create_early_exit(ExitCode::OptionRequiresArgument);
}

auto OptionParser::parse_long(Token const& token) -> Action {
	auto const word = token.lexeme.substr(2);

	if (word == "version" && m_context.has_version()) {
		m_context.print_version();
		return Result::create_early_exit(ExitCode::Success);
	}

	if (word == "help") {
		m_context.print_help_text();
		return Result::create_early_exit(ExitCode::Success);
	}

	auto const split = ArgSplit::create(word);
	auto const* named = find_named(split.left);

	if (!named) {
		m_context.print_error(std::format("unrecognized option: '{}'", token.lexeme));
		return Result::create_early_exit(ExitCode::UnrecognizedOption);
	}

	auto const ret = parse_option_value(*named, split.right, token.lexeme);
	return or_iterate(ret, Iterate::Consume);
}

auto OptionParser::parse_shorts(Token const& token) -> Action {
	auto const word = token.lexeme.substr(1);
	auto const split = ArgSplit::create(word);
	auto letters = split.left;
	for (; letters.size() > 1; letters.remove_prefix(1)) {
		auto const* named = find_named(letters.front());
		if (!named) {
			return Result::create_early_exit(ExitCode::UnrecognizedOption);
		}

		if (!named->is_flag) {
			m_context.print_error(std::format("option requires an argument -- '{}'", named->letter));
			return Result::create_early_exit(ExitCode::OptionRequiresArgument);
		}

		named->parse("true");
	}

	auto const* named = find_named(letters.front());
	if (!named) {
		return Result::create_early_exit(ExitCode::UnrecognizedOption);
	}

	auto const ret = parse_option_value(*named, split.right, token.lexeme);
	return or_iterate(ret, Iterate::Consume);
}

auto OptionParser::parse_option_value(parameter::Named const& named,
									  std::string_view const value,
									  std::string_view const option_lexeme) -> Result {
	if (!named.is_flag && value.empty()) {
		m_expects_value = &named;
		m_option_lexeme = option_lexeme;
		return Result::create_continue();
	}

	if (!value.empty()) {
		return assign_value_to(named, value, option_lexeme);
	}

	if (!named.is_flag) {
		m_context.print_error(std::format("option '{}' requires an argument", option_lexeme));
		return Result::create_early_exit(ExitCode::OptionRequiresArgument);
	}

	named.parse("true");
	return Result::create_continue();
}

auto OptionParser::assign_value_to(parameter::Named const& named,
								   std::string_view const value,
								   std::string_view const option_lexeme) const -> Result {
	if (!named.parse(value)) {
		m_context.print_error(std::format("invalid '{}': {}", option_lexeme, value));
		return Result::create_early_exit(ExitCode::InvalidArgument);
	}

	return Result::create_continue();
}

template <typename T>
auto OptionParser::find_named(T const t) const -> parameter::Named const* {
	auto const pred = [t](parameter::Named const& param) {
		if constexpr (std::same_as<T, char>) {
			return param.letter == t;
		} else {
			return param.word == t;
		}
	};
	auto const it = std::ranges::find_if(m_options, pred);
	if (it != m_options.end()) {
		return &*it;
	}

	m_context.print_error(std::format("unrecognized option: '{}'", t));

	return nullptr;
}
} // namespace clap
