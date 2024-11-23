#pragma once
#include <clap/app_info.hpp>
#include <clap/arg.hpp>
#include <clap/build_version.hpp>
#include <clap/result.hpp>

namespace clap {
[[nodiscard]] auto parse(AppInfo const& info, std::span<Arg const> args, int argc, char const* const* argv) -> Result;
}
