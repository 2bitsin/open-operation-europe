#include "opeu/assets/layered-pack.hpp"

#include "opeu/assets/fallback-pack.hpp"

#include <utility>

namespace opeu::assets::detail::layered_pack {
namespace {

auto Over(std::unique_ptr<AssetPack const> base, Pack remaster) -> std::unique_ptr<AssetPack const> {
  return std::make_unique<FallbackPack const>(std::make_unique<Pack const>(std::move(remaster)), std::move(base));
}

}

auto OpenLayered(std::unique_ptr<AssetPack const> base, std::unique_ptr<PackFiles const> remaster) -> Packed<std::unique_ptr<AssetPack const>> {
  auto over{ Pack::Open(std::move(remaster)) };
  if (!over && over.error().fault == PackFault::MISSING_FILE) return base;
  if (!over) return std::unexpected{ over.error() };
  return Over(std::move(base), *std::move(over));
}

auto OpenLayered(std::unique_ptr<AssetPack const> base, std::optional<std::filesystem::path> const& remaster) -> Packed<std::unique_ptr<AssetPack const>> {
  if (!remaster) return base;
  auto over{ OpenPack(*remaster) };
  if (!over) return std::unexpected{ over.error() };
  return Over(std::move(base), *std::move(over));
}

}
