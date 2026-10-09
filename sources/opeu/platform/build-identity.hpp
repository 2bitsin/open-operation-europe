#pragma once
// What this build of the port is, as buildutil stamped it before configure
// (_bdudata/buildinfo.json): the line a --version flag and a crash report say.

#include <cstdint>
#include <string>
#include <string_view>

namespace opeu::platform::detail::build_identity {

struct BuildIdentity {
  std::string_view version{ };  // `git describe --tags --always --dirty`
  std::string_view commit{ };   // short hash, "unknown" outside git
  std::string_view tag{ };      // exact tag of the commit, empty when none
  std::string_view time{ };     // UTC, "YYYY-MM-DDTHH:MMZ"
  bool             dirty{ };    // tracked files differed from the commit
  std::uint32_t    number{ };   // this checkout's build counter

  auto operator==(BuildIdentity const&) const noexcept -> bool = default;
};

[[nodiscard]] auto ThisBuild() noexcept -> BuildIdentity const&;

// "opeurptv <version> (build <number>, <time>)"; a missing time is left out.
[[nodiscard]] auto Describe(BuildIdentity const& build) -> std::string;

}

namespace opeu::platform {
using detail::build_identity::BuildIdentity;
using detail::build_identity::Describe;
using detail::build_identity::ThisBuild;
}
