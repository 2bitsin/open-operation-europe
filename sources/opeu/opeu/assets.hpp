// The original data files compiled into the executable (assets.embed/), as the lookup the services' file layer takes
// (services/asset-table.hpp).
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace opeu::opeu::detail::assets {

// A DOS file name -> the shipped bytes: the drive and directories are dropped and the name upper-cased (DOS names are
// case-blind); nothing when that file was not shipped.
[[nodiscard]] auto AssetFind(std::string_view dos_name) -> std::optional<std::span<std::uint8_t const>>;

}

namespace opeu::opeu {
using detail::assets::AssetFind;
}
