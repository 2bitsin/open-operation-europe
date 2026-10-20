#pragma once
// A remaster laid over a base pack. The base is whatever the game draws without one: the classic pack compiled into the
// executable, or the decoders' pack over the player's data (CataloguePack). A remaster's manifest is self-contained, so
// it lays over either; FallbackPack answers every id the remaster does not hold from the base.

#include "opeu/assets/pack-files.hpp"
#include "opeu/assets/pack.hpp"

#include <filesystem>
#include <memory>
#include <optional>

namespace opeu::assets::detail::layered_pack {

// The remaster `files` over `base`, or `base` alone when the files hold no pack.json (no remaster embedded). A remaster
// that has a manifest but cannot be read is an error, not a silent fall back.
[[nodiscard]] auto OpenLayered(std::unique_ptr<AssetPack const> base, std::unique_ptr<PackFiles const> remaster) -> Packed<std::unique_ptr<AssetPack const>>;

// The remaster at `remaster` (a directory or an .opeupack) over `base`, or `base` alone when there is none. A path that
// holds no readable pack is an error: the player asked for that remaster.
[[nodiscard]] auto OpenLayered(std::unique_ptr<AssetPack const> base, std::optional<std::filesystem::path> const& remaster) -> Packed<std::unique_ptr<AssetPack const>>;

}

namespace opeu::assets {
using detail::layered_pack::OpenLayered;
}
