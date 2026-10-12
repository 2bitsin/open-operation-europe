#include "opeu/assets/fallback-pack.hpp"

#include "opeu/contract.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace opeu::assets::detail::fallback_pack {

FallbackPack::FallbackPack(std::unique_ptr<AssetPack const> preferred, std::unique_ptr<AssetPack const> fallback) : _preferred{ std::move(preferred) }, _fallback{ std::move(fallback) } {
  Expects(_preferred != nullptr && _fallback != nullptr, "a fallback pack stacks two packs");
}

auto FallbackPack::Find(std::string_view const id) const -> std::optional<PackedAsset> {
  return Holder(id).Find(id);
}

auto FallbackPack::Load(std::string_view const id) const -> Packed<LoadedAsset> {
  return Holder(id).Load(id);
}

auto FallbackPack::Palette(std::string_view const name) const -> std::optional<PaletteColours> {
  return _preferred->Palette(name).or_else([&] { return _fallback->Palette(name); });
}

auto FallbackPack::Ids() const -> std::vector<std::string> {
  auto const preferred{ _preferred->Ids() };
  auto const fallback{ _fallback->Ids() };
  auto       ids{ std::vector<std::string>{} };
  std::ranges::set_union(preferred, fallback, std::back_inserter(ids));
  return ids;
}

auto FallbackPack::Holder(std::string_view const id) const -> AssetPack const& {
  return _preferred->Find(id) ? *_preferred : *_fallback;
}

}
