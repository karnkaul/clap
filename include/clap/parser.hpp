#pragma once
#include <string>
#include <vector>

namespace clap {
///
/// \brief Command line arguments parser
///
template <typename Ch = char>
class basic_parser;

using parser = basic_parser<>;

template <typename Ch>
class basic_parser {
  public:
	using char_t = Ch;
	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;
	///
	/// \brief Option structure
	///
	struct option_t {
		///
		/// \brief Word / letter identifying option
		///
		string_t id;
		///
		/// \brief Value for option (if any)
		///
		string_t value;
	};
	///
	/// \brief Command structure
	///
	struct command_t {
		///
		/// \brief Word / letter identifying command
		///
		string_t id;
		std::vector<option_t> options;
	};
	///
	/// \brief Expression structure
	///
	struct expr_t {
		command_t command;
		std::vector<option_t> options;
		std::vector<string_t> arguments;
	};

	///
	/// \brief Parse a container of string-like objects
	///
	template <typename C>
	expr_t parse(C const& input, std::size_t start);
	///
	/// \brief Parse a vector of C strings
	///
	expr_t parse(std::vector<char_t const*> const& input, std::size_t start);
	///
	/// \brief Parse raw args
	///
	expr_t parse(int argc, char_t const* const argv[], int start = 1);

  private:
	enum class state { start, cmd, args };

	void next(int argc, char_t const* const argv[]);
	void options(char_t const* const argv[]);

	struct {
		expr_t expr;
		state state_{};
		int idx;
	} m_data = {};
};

// impl

template <typename Ch>
template <typename C>
typename basic_parser<Ch>::expr_t basic_parser<Ch>::parse(C const& input, std::size_t start) {
	std::vector<char_t const*> vec;
	vec.reserve(input.size());
	for (auto const& str : input) {
		vec.push_back(str.data());
	}
	return parse(vec, start);
}

template <typename Ch>
typename basic_parser<Ch>::expr_t basic_parser<Ch>::parse(std::vector<char_t const*> const& input, std::size_t start) {
	return parse(int(input.size()), input.data(), int(start));
}

template <typename Ch>
void basic_parser<Ch>::options(char_t const* const argv[]) {
	auto& vec = m_data.state_ == state::cmd ? m_data.expr.command.options : m_data.expr.options;
	auto str = string_view_t(argv[std::size_t(m_data.idx)]);
	auto const eq = str.find('=');
	if (str.size() > 2 && str[1] == '-') {
		if (eq < str.size()) {
			vec.push_back({string_t(str.substr(2, eq - 2)), string_t(str.substr(eq + 1))});
		} else {
			vec.push_back({string_t(str.substr(2)), {}});
		}
	} else {
		str = str.substr(1);
		option_t* last = {};
		while (!str.empty() && str[0] != '=') {
			vec.push_back({string_t(str.substr(0, 1)), {}});
			str = str.substr(1);
			last = &vec.back();
		}
		if (!str.empty() && last) {
			last->value = str.substr(1);
		}
	}
}

template <typename Ch>
void basic_parser<Ch>::next(int argc, char_t const* const argv[]) {
	if (auto str = string_view_t(argv[std::size_t(m_data.idx)]); !str.empty()) {
		if (m_data.state_ == state::args) {
			m_data.expr.arguments.emplace_back(str);
		} else {
			if (str[0] == '-') {
				options(argv);
			} else {
				if (m_data.expr.command.id.empty()) {
					m_data.expr.command.id = str;
					m_data.state_ = state::cmd;
				} else {
					m_data.expr.arguments.emplace_back(str);
					m_data.state_ = state::args;
				}
			}
		}
	}
	if (++m_data.idx < argc) {
		next(argc, argv);
	}
}

template <typename Ch>
typename basic_parser<Ch>::expr_t basic_parser<Ch>::parse(int argc, char_t const* const argv[], int start) {
	m_data = {};
	if (start < argc) {
		m_data.idx = start;
		next(argc, argv);
	}
	return std::move(m_data.expr);
}
} // namespace clap
