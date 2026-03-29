#pragma once
#include <string_view>

namespace clap {
class IPrinter {
  public:
	virtual ~IPrinter() = default;

	IPrinter() = default;
	IPrinter(IPrinter const&) = default;
	IPrinter(IPrinter&&) = default;
	IPrinter& operator=(IPrinter const&) = default;
	IPrinter& operator=(IPrinter&&) = default;

	[[nodiscard]] static auto default_printer() -> IPrinter&;

	virtual void println(std::string_view message) = 0;
	virtual void printerr(std::string_view message) = 0;
};
} // namespace clap
