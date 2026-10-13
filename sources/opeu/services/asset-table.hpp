// The shipped data files, compiled into the executable: the application (opeu/assets.cpp) hands the file service
// (fileio.cpp) its lookup before the game starts. A registration, not a link edge: the services cannot depend on the
// application module.
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace opeu::services::detail::asset_table {

// A DOS file name ("SCEDATA", "c:\\opeu\\msg.16p") -> the shipped bytes, if that file was shipped.
using AssetLookup = std::function<std::optional<std::span<std::uint8_t const>>(std::string_view dos_name)>;

auto FilesUseAssets(AssetLookup lookup) -> void;

}

namespace opeu::services {
using detail::asset_table::AssetLookup;
using detail::asset_table::FilesUseAssets;
}
