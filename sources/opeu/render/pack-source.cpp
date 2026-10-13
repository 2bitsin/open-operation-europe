#include "opeu/render/pack-source.hpp"

#include <format>

namespace opeu::render::detail::pack_source {

PackSource::PackSource(assets::AssetPack const& pack, assets::Manifest const& listing) : _pack{ pack }, _listing{ listing } { }

auto PackSource::Load(std::string_view const id) const -> Rendered<assets::LoadedAsset> {
  auto loaded{ _pack.get().Load(id) };
  if (!loaded) return PackFailed(loaded.error());
  return *std::move(loaded);
}

PackSource::PackSource(assets::Pack const& pack) : PackSource{ pack, pack.Listing() } { }

auto PackSource::Palette(std::string_view const name) const -> Rendered<assets::PaletteColours> {
  if (auto const colours{ _pack.get().Palette(name) }) return *colours;
  return Failed(std::format("no palette {}", name));
}

auto PackSource::Frame(std::string_view const animation, std::size_t const frame) const -> Rendered<std::string> {
  auto const frames{ _listing.get().Frames(animation) };
  if (!frames) return Failed(std::format("no animation {}", animation));
  if (frame >= frames->size()) return Failed(std::format("{} has {} frames, not {}", animation, frames->size(), frame + 1));
  return frames->at(frame);
}

auto PackSource::Font(std::string_view const name) const -> Rendered<assets::Font> {
  if (auto const font{ _listing.get().FontNamed(name) }) return *font;
  return Failed(std::format("no font {}", name));
}

}
