#pragma once
#include <string_view>

namespace cliq {
/// \brief Application info.
struct AppInfo {
	/// \brief One liner app description.
	std::string_view help_text{};
	/// \brief Version text.
	std::string_view version{"unknown"};

	/// \brief Help text epilogue.
	std::string_view epilogue{};
};
} // namespace cliq
