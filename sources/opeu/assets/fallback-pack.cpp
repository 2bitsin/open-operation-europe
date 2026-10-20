#include "opeu/assets/fallback-pack.hpp"

#include "opeu/contract.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace opeu::assets::detail::fallback_pack {

namespace {

// The names of two sorted lists, sorted, each once.
auto Union(std::vector<std::string> const& first, std::vector<std::string> const& second) -> std::vector<std::string> {
  auto names{ std::vector<std::string>{} };
  std::ranges::set_union(first, second, std::back_inserter(names));
  return names;
}

}

FallbackPack::FallbackPack(std::unique_ptr<AssetPack const> preferred, std::unique_ptr<AssetPack const> fallback) : _preferred{ std::move(preferred) }, _fallback{ std::move(fallback) } {
  Expects(_preferred != nullptr && _fallback != nullptr, "a fallback pack stacks two packs");
}

auto FallbackPack::Find(std::string_view const id) const -> std::optional<PackedAsset> {
  return Holder(id).Find(id);
}

auto FallbackPack::Lists(std::string_view const id) const -> bool {
  return _preferred->Lists(id) || _fallback->Lists(id);
}

auto FallbackPack::Load(std::string_view const id) const -> Packed<LoadedAsset> {
  return Holder(id).Load(id);
}

auto FallbackPack::Palette(std::string_view const name) const -> std::optional<PaletteColours> {
  return _preferred->Palette(name).or_else([&] { return _fallback->Palette(name); });
}

auto FallbackPack::Ids() const -> std::vector<std::string> {
  return Union(_preferred->Ids(), _fallback->Ids());
}

auto FallbackPack::Frames(std::string_view const animation) const -> std::optional<std::vector<std::string>> {
  return _preferred->Frames(animation).or_else([&] { return _fallback->Frames(animation); });
}

auto FallbackPack::FontNamed(std::string_view const name) const -> std::optional<Font> {
  return _preferred->FontNamed(name).or_else([&] { return _fallback->FontNamed(name); });
}

auto FallbackPack::FontNames() const -> std::vector<std::string> {
  return Union(_preferred->FontNames(), _fallback->FontNames());
}

auto FallbackPack::Scale() const -> std::uint32_t {
  return _preferred->Scale();
}

auto FallbackPack::Holder(std::string_view const id) const -> AssetPack const& {
  return _preferred->Lists(id) ? *_preferred : *_fallback;
}

}
