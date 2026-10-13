#pragma once
// Turning a pack's asset into the pixels of a texture for a palette. An asset the game recolours carries an index map
// and is baked in the palette's colours; any other is its RGBA image as the pack holds it.

#include "opeu/assets/manifest.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/assets/texture.hpp"
#include "opeu/render/geometry.hpp"

#include <algorithm>
#include <cstddef>

namespace opeu::render::detail::baking {

// Every index white: an ink shape (a glyph, a logo) baked so colour modulation draws it in any pen colour.
inline constexpr auto INK{ [] {
  auto colours{ assets::PaletteColours{} };
  std::ranges::fill(colours, assets::Colour{ .r = 0xFF, .g = 0xFF, .b = 0xFF });
  return colours;
}() };

[[nodiscard]] auto Bake(assets::LoadedAsset const& asset, assets::PaletteColours const& colours) -> assets::Texture;

// A palette entry as a pen colour.
[[nodiscard]] auto Pen(assets::PaletteColours const& colours, std::size_t index) -> Rgba;

}

namespace opeu::render {
using detail::baking::Bake;
using detail::baking::INK;
using detail::baking::Pen;
}
