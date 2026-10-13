#pragma once
// What the renderer asks of an asset pack: images by id, palettes, animations' frames and fonts. An AssetPack answers
// the first two; the listing (a Pack's manifest) the last two, which AssetPack does not expose yet.

#include "opeu/assets/manifest.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/render/render-error.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace opeu::render::detail::pack_source {

class PackSource {
public:
  // `pack` for the images and palettes, `listing` for the animations and fonts (the bottom pack's, under a remaster).
  PackSource(assets::AssetPack const& pack, assets::Manifest const& listing);
  explicit PackSource(assets::Pack const& pack);

  [[nodiscard]] auto Load(std::string_view id) const -> Rendered<assets::LoadedAsset>;
  [[nodiscard]] auto Palette(std::string_view name) const -> Rendered<assets::PaletteColours>;
  // The id of an animation's frame.
  [[nodiscard]] auto Frame(std::string_view animation, std::size_t frame) const -> Rendered<std::string>;
  [[nodiscard]] auto Font(std::string_view name) const -> Rendered<assets::Font>;

private:
  std::reference_wrapper<assets::AssetPack const> _pack;
  std::reference_wrapper<assets::Manifest const>  _listing;
};

}

namespace opeu::render {
using detail::pack_source::PackSource;
}
