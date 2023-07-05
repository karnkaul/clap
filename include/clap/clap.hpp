#pragma once
#include <clap/build_version.hpp>
#include <clap/detail/from_string.hpp>
#include <concepts>
#include <memory>
#include <span>

namespace clap {
///
/// \brief Constraint for a type that can be parsed from a string(stream).
///
/// operator>>(istream&, T&) must be overloaded.
///
template <typename Type>
concept ParseableT = requires(std::istream& i, Type& t) {
	{ i >> t } -> std::convertible_to<std::istream&>;
};

namespace detail {
template <typename Type>
constexpr bool is_bindable_v{false};

template <ParseableT Type>
constexpr bool is_bindable_v<Type>{true};

template <ParseableT Type>
static constexpr bool is_bindable_v<std::vector<Type>>{true};
} // namespace detail

///
/// \brief Constraint for a type that can be bound as an argument.
///
/// Must be ParseableT or std::vector<ParseableT>.
///
template <typename Type>
concept BindableT = detail::is_bindable_v<Type>;

///
/// \brief Result of parsing options.
///
enum class Result { eContinue, eExit, eParseError };

///
/// \brief Primary interface to build options and parse command line arguments.
///
class Options {
  public:
	///
	/// \brief Construct an Options instance.
	/// \param app_name Name of application (displayed in help and on errors).
	/// \param description Description of application (displayed in help).
	/// \param version Version text to display.
	///
	Options(std::string app_name, std::string description, std::string version);

	///
	/// \brief Set the (optional) help footer text.
	/// \param footer Text to display.
	///
	Options& set_footer(std::string footer);

	///
	/// \brief Bind an option with an implicit boolean value.
	/// \param out_flag Flag to set if option is passed.
	/// \param group Group of option ("<key>" or "<letter>,<key>").
	/// \param description Description of option (displayed in help).
	///
	Options& flag(bool& out_flag, std::string group, std::string description);

	///
	/// \brief Bind an option with a required argument.
	/// \param out Variable to parse argument into.
	/// \param group Group of option ("<key>" or "<letter>,<key>").
	/// \param description Description of option (displayed in help).
	/// \param usage Usage text.
	///
	template <BindableT Type>
	Options& required(Type& out, std::string group, std::string description, std::string usage) {
		auto arg = detail::Argument{.usage = std::move(usage), .from_string = detail::make_from_string(out), .required = true};
		return bind_option({}, std::move(arg), std::move(group), std::move(description));
	}

	///
	/// \brief Bind an option with an optional argument.
	/// \param out Variable to parse argument into, if passed.
	/// \param group Group of option ("<key>" or "<letter>,<key>").
	/// \param description Description of option (displayed in help).
	/// \param usage Usage text.
	/// \param out_was_passed Flag that's set if option is passed, regardless of argument presence.
	///
	template <BindableT Type>
	Options& optional(Type& out, bool& out_was_passed, std::string group, std::string description, std::string usage) {
		auto arg = detail::Argument{.usage = std::move(usage), .from_string = detail::make_from_string(out)};
		return bind_option(&out_was_passed, std::move(arg), std::move(group), std::move(description));
	}

	///
	/// \brief Bind the next positional argument.
	/// \param out Variable to parse argument into.
	/// \param operand Name of operand.
	/// \param usage Usage text, if different from operand.
	///
	template <ParseableT Type>
	Options& positional(Type& out, std::string operand, std::string usage = {}) {
		return bind_positional(detail::make_from_string(out), std::move(operand), std::move(usage));
	}

	///
	/// \brief Bind (all) unmatched arguments.
	/// \param out Vector to fill with unmatched arguments.
	/// \param usage Usage text.
	///
	template <ParseableT Type>
	Options& unmatched(std::vector<Type>& out, std::string usage) {
		return bind_unmatched(detail::make_from_string(out), std::move(usage));
	}

	///
	/// \brief Parse a range of C strings.
	/// \returns Result of parsing.
	/// \param args range of strings to parse.
	///
	Result parse(std::span<char const* const> args);

	///
	/// \brief Parse arguments to main.
	/// \returns Result of parsing.
	/// \param argc First argument to main.
	/// \param argv Second argument to main.
	/// \param parse_arg0 Whether to parse the argv[0] as an option (otherwise skip it).
	///
	Result parse(int argc, char const* const* argv, bool const parse_arg0 = false);

  private:
	Options& bind_option(bool* out_was_passed, detail::Argument argument, std::string group, std::string description);
	Options& bind_positional(detail::FromString from_string, std::string key, std::string usage);
	Options& bind_unmatched(detail::FromString from_string, std::string usage);
	void print_help();

	struct Impl;
	struct Deleter {
		void operator()(Impl const* ptr) const;
	};

	std::unique_ptr<Impl, Deleter> m_impl{};
};

///
/// \brief Check if application should quit.
///
constexpr bool should_quit(Result const result) { return result != Result::eContinue; }

///
/// \brief Obtain the return code.
/// \returns EXIT_FAILURE if parse_error(), else EXIT_SUCCESS.
///
constexpr int return_code(Result const result) { return result == Result::eParseError ? EXIT_FAILURE : EXIT_SUCCESS; }

///
/// \brief Obtain the app name from the first argument to main.
/// \returns std::filesystem::path(arg0).stem().
///
std::string make_app_name(std::string_view const arg0);
} // namespace clap
