#pragma once
#include <token.hpp>
#include <span>

namespace clap {
class Scanner {
  public:
	explicit constexpr Scanner(std::span<char const* const> args) : m_args(args) {
		if (!m_args.empty()) { m_next = to_token(m_args.front()); }
	}

	constexpr auto next() -> bool {
		advance();
		if (m_current.token.token_type == TokenType::ForceArgs) { m_force_args = true; }
		return m_current.token.token_type != TokenType::None;
	}

	[[nodiscard]] constexpr auto get_args() const -> std::span<char const* const> { return m_args; }

	[[nodiscard]] constexpr auto peek() const -> TokenType { return m_next.token_type; }

	[[nodiscard]] constexpr auto get_token_type() const -> TokenType { return m_current.token.token_type; }
	[[nodiscard]] constexpr auto get_option_type() const -> OptionType { return m_current.token.option_type; }

	[[nodiscard]] constexpr auto get_key() const -> std::string_view { return m_current.key; }
	[[nodiscard]] constexpr auto get_value() const -> std::string_view { return m_current.value; }

	constexpr auto next_letter(char& out_letter, bool& out_is_last) -> bool {
		if (m_current.token.option_type != OptionType::Letters || m_current.key.empty()) { return false; }
		out_letter = m_current.key.front();
		m_current.key = m_current.key.substr(1);
		out_is_last = m_current.key.empty();
		return true;
	}

	constexpr auto next_letter(char& out_letter) -> bool {
		auto is_last = false;
		return next_letter(out_letter, is_last);
	}

  private:
	constexpr void advance() {
		if (m_next.token_type == TokenType::None) {
			m_current = {};
			return;
		}
		m_current.token = m_next;
		set_key_value();
		set_next();
	}

	constexpr void set_next() {
		if (m_args.size() <= 1) {
			m_args = {};
			m_next = {};
			return;
		}
		m_next = to_token(m_args[1]);
		if (m_force_args) { m_next.token_type = TokenType::Argument; }
		m_args = m_args.subspan(1);
	}

	constexpr void set_key_value() {
		m_current.key = m_current.value = {};
		if (!m_force_args && m_current.token.token_type == TokenType::Option) {
			m_current.key = m_current.token.value;
			auto const eq = m_current.key.find_first_of('=');
			if (eq != std::string_view::npos) {
				m_current.value = m_current.key.substr(eq + 1);
				m_current.key = m_current.key.substr(0, eq);
			}
		} else {
			m_current.value = m_current.token.value;
		}
	}

	std::span<char const* const> m_args{};

	struct {
		Token token{};
		std::string_view key{};
		std::string_view value{};
	} m_current{};
	Token m_next{};
	bool m_force_args{};
};
} // namespace clap
