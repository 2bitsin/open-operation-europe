#include "opeu/render/baking.hpp"

namespace opeu::render::detail::baking {

auto Bake(assets::LoadedAsset const& asset, assets::PaletteColours const& colours) -> assets::Texture {
  if (asset.indexed) return assets::ToTexture(*asset.indexed, colours);
  return asset.texture;
}

auto Pen(assets::PaletteColours const& colours, std::size_t const index) -> Rgba {
  auto const colour{ colours.at(index) };
  return Rgba{ .r = colour.r, .g = colour.g, .b = colour.b };
}

}
