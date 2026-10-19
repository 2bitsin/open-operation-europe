// The original data files compiled into the executable (assets.embed/), as the lookup the services' file layer takes
// (services/asset-table.hpp).
#pragma once

#include "opeu/assets/source.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace opeu::app::detail::assets {

// A DOS file name -> the shipped bytes: the drive and directories are dropped and the name upper-cased (DOS names are
// case-blind); nothing when that file was not shipped.
[[nodiscard]] auto AssetFind(std::string_view dos_name) -> std::optional<std::span<std::uint8_t const>>;
// Every shipped file by its name, for the decoders (the game's screen draws the classic pack from them).
[[nodiscard]] auto ShippedFiles() -> ::opeu::assets::TableSource;

}

namespace opeu::app {
using detail::assets::AssetFind;
using detail::assets::ShippedFiles;
}
