// What the build compiled into the executable (configure.py): the 26 data files (assets.embed/) and the classic pack
// with any remaster (packs.embed/), each when its directory is in the tree. The private repository holds both; the
// public mirror holds neither, and the game then reads the player's data directory and derives the pack from it.
#pragma once

#include "opeu/assets/game-data.hpp"
#include "opeu/assets/pack-files.hpp"
#include "opeu/assets/pack.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace opeu::app::detail::embedded {
using namespace std::string_view_literals;

inline constexpr auto CLASSIC_PACK{ "classic"sv };
inline constexpr auto REMASTER_PACK{ "remaster"sv };

// The 26 data files as compiled in, by name; nothing when the build has none.
[[nodiscard]] auto EmbeddedData() -> std::optional<assets::GameData::Table>;

// The files of the embedded pack in packs.embed/<directory>/; none at all when the build has no pack.
[[nodiscard]] auto EmbeddedPackFiles(std::string_view directory) -> std::unique_ptr<assets::PackFiles const>;

// The embedded pack in packs.embed/<directory>/; nothing when the build has no pack.
[[nodiscard]] auto OpenEmbeddedPack(std::string_view directory) -> std::optional<assets::Packed<assets::Pack>>;

}

namespace opeu::app {
using detail::embedded::CLASSIC_PACK;
using detail::embedded::EmbeddedData;
using detail::embedded::EmbeddedPackFiles;
using detail::embedded::OpenEmbeddedPack;
using detail::embedded::REMASTER_PACK;
}
