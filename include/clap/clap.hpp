#pragma once
#include <ostream>
#include <clap/clap_types.hpp>

namespace clap {
///
/// \brief Result of parsing
///
enum class parse_result { run, quit, parse_error };
using str_t = std::string_view;

struct option_parser;
struct root_parser;
struct program_spec;

///
/// \brief Optional customization for root parser and output (defaults to std::cout and std::cerr)
///
struct parse_info {
	///
	/// \brief Custom root parser to use (optional)
	///
	root_parser* root{};
	///
	/// \brief Custom standard out (optional)
	///
	std::ostream* cout{};
	///
	/// \brief Custom standard error (optional)
	///
	std::ostream* cerr{};
};

///
/// \brief Parse command line args
/// \param spec program specification
/// \param argc number of arguments (passed directly to main())
/// \param argv pointer to arguments C-vector of C-strings (passed directly to main())
/// \param info custom root parser / std out / err, etc
/// \returns on successful parsing: quit if any parser requested it, else run; otherwise parse_error
///
/// Syntax: [option...] [arg...] [cmd...]
/// cmd: cmd_key [option...] [arg...]
/// cmd_key (regex): [A-z]+
/// option (no / optional argument): -keylist | -option_key[=arg] | --option_name[=arg]
/// option (required argument): -option_keyarg | -option_key arg | -option_key=arg | --option_name=arg
/// option_key (regex): [A-z]
/// option_name (regex): [A-z]+
/// keylist (only options without arguments): -option_key0[option_key1...]
///
parse_result parse_args(program_spec const& spec, int argc, char const* const argv[], parse_info const& info = {});

///
/// \brief Option specification
///
struct option {
	enum flag_t { flag_none = 0, flag_optional = 1 << 0 };
	using flags_t = std::uint32_t;

	///
	/// \brief Long name
	///
	str_t name;
	///
	/// \brief Argument doc, if any
	///
	str_t arg;
	///
	/// \brief Doc string
	///
	str_t doc;
	///
	/// \brief Identifying key; if char, also used as short-code
	///
	option_key key;
	///
	/// \brief Meta flags
	///
	flags_t flags{};

	constexpr option(option_key key, str_t name, str_t doc = {}, str_t arg = {}, flags_t flags = {}) noexcept
		: name(name), arg(arg), doc(doc), key(key), flags(flags) {}
};
using options_view = array_view<option>;

///
/// \brief List of cmds
///
using cmds_view = array_view<option_parser*>;

///
/// \brief Program specification
///
struct program_spec {
	///
	/// \brief Version string
	///
	str_t version;
	///
	/// \brief Doc string
	///
	str_t doc;
	///
	/// \brief Name (assigned through arg0 if empty)
	///
	mutable str_t name;
	///
	/// \brief Primary option parser (required)
	///
	option_parser* parser{};
	///
	/// \brief Verbs, if any (with secondary parsers)
	///
	/// The internal parser supports contextually switching the active option_parser via a "cmd" as a non-option argument.
	/// Such parses must be associated to unique keys and must not have option key collisions.
	/// Only one parser is active at a time (in addition to the root parser) for (non) options being parsed; a cmd simply modifies that.
	/// Set this member to an array of such cmds if desired.
	///
	cmds_view cmds;
	///
	/// \brief Minimum number of cmds to have been invoked for success (only applicable if cmds is non empty)
	///
	int cmds_required{};
};

///
/// \brief View into parse state metadata (passed to option_parser callback)
///
/// Can only be constructed by clap internals
///
class parse_state {
  public:
	///
	/// \brief Obtain const reference to root parser
	///
	parse_info const& info() const noexcept;
	///
	/// \brief Obtain const reference to active parser
	///
	option_parser const& parser() const noexcept;
	///
	/// \brief Obtain number of options parsed so far
	///
	int opts_parsed() const noexcept;
	///
	/// \brief Obtain number of cmds parsed so far
	///
	int cmds_parsed() const noexcept;
	///
	/// \brief Obtain current argument index
	///
	int arg_index() const noexcept;
	///
	/// \brief Obtain active cmd key, if any
	///
	str_t cmd() const noexcept;
	///
	/// \brief Check if the quit flag been set
	///
	bool quit() const noexcept;
	///
	/// \brief Terminate parsing after this argument
	/// \param set_quit set the quit flag (not modified if false)
	///
	void early_exit(bool set_quit) noexcept;

  private:
	parse_state(struct state_impl* impl) noexcept : m_impl(impl) {}
	struct state_impl* m_impl;
	friend struct state_impl;
	friend struct option_parser;
};

///
/// \brief Option parser customization point
///
struct option_parser {
	///
	/// \brief Unscoped enum for identifying non-option args
	///
	/// no_arg: argument to (preceding) option
	/// no_end: termination token (all arguments have been parsed)
	///
	enum non_opt { no_arg = -100, no_end };
	///
	/// \brief Parser specification
	///
	struct spec_t {
		///
		/// \brief Option list
		///
		options_view options;
		///
		/// \brief Args doc string, if any
		///
		str_t arg_doc;
		///
		/// \brief Description for command / args in arg_doc
		///
		str_t doc_desc;
		///
		/// \brief Unique identifier for this parser (if not primary)
		///
		str_t cmd_key;
		///
		/// \brief Doc string for cmd
		///
		str_t cmd_doc;
	};

	///
	/// \brief Instance specification (initialize before operator(), eg in constructor)
	///
	spec_t spec;

	///
	/// \brief Customization point
	///
	/// This function is called once for every argument being parsed.
	/// If the argument is an option, its key is passd as the identifier.
	/// If the option takes an argument, it will be passed in arg; if not optional and missing, an error will be logged.
	/// If the argument is a non-option, the key is option_parser::non_opt::no_arg.
	/// This function is also called once at the end with key == option_parser::non_opt::no_end.
	/// The function should return false to print corresponding parse errors, and true to signal the parser to continue.
	/// Typical impementations handle all options (and args) within a single switch block.
	/// The current parse state can be inspected and set to exit and/or terminate parsing.
	/// Eg: to suppress errors, call state.early_exit() and return true to stop parsing.
	///
	virtual bool operator()(option_key key, str_t arg, parse_state state) = 0;

	///
	/// \brief Helper function to print (short) usage
	///
	static void usage(parse_state state);
};

///
/// \brief Interface for root parser (expected to handle help, usage, version, etc)
///
/// Root parser is always called and only for options (never for non-options / termination).
/// Its options should not have char keys.
/// Its doc strings are appended to the end of the active parser's.
///
struct root_parser : option_parser {
	program_spec const* program{};

	option_parser* parser() const noexcept { return program->parser; }
};

///
/// \brief Obtain clap version
///
str_t lib_version() noexcept;
} // namespace clap
