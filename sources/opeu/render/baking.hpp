#pragma once
// Turning a pack's asset into the pixels of a texture for a palette. An asset the game recolours carries an index map
// and is baked in the palette's colours; any other is its RGBA image as the pack holds it. Either can be keyed: the
// pixels of colour 0 let the screen show, as the game's transparent copies draw them.

#include "opeu/assets/manifest.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/assets/texture.hpp"
#include "opeu/render/geometry.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace opeu::render::detail::baking {

// Every index white: an ink shape (a glyph, a logo) baked so colour modulation draws it in any pen colour.
inline constexpr auto INK{ [] {
  auto colours{ assets::PaletteColours{} };
  std::ranges::fill(colours, assets::Colour{ .r = 0xFF, .g = 0xFF, .b = 0xFF });
  return colours;
}() };

enum class Keying : std::uint8_t {
  OPAQUE,      // every pixel the image draws
  COLOUR_ZERO, // pixels of colour 0 let the screen show
};

// How an asset is baked: in `colours`, keyed or not. An image the pack holds only as RGBA keeps its own colours, and
// keying it lets the pixels of `background` (its own palette's colour 0) show the screen.
struct Painting {
  assets::PaletteColours        colours{};
  Keying                        keying{ Keying::OPAQUE };
  std::optional<assets::Colour> background;
};

[[nodiscard]] auto Bake(assets::LoadedAsset const& asset, assets::PaletteColours const& colours) -> assets::Texture;
[[nodiscard]] auto Bake(assets::LoadedAsset const& asset, Painting const& painting) -> assets::Texture;

// The image with its colour 0 masked out as well, when keyed.
[[nodiscard]] auto KeyedImage(assets::Image const& image, Keying keying) -> assets::Image;
// The RGBA image with the pixels of `background` made transparent, when keyed and a background is known.
[[nodiscard]] auto KeyedImage(assets::Texture const& image, Keying keying, std::optional<assets::Colour> background) -> assets::Texture;

// A palette entry as a pen colour.
[[nodiscard]] auto Pen(assets::PaletteColours const& colours, std::size_t index) -> Rgba;

}

namespace opeu::render {
using detail::baking::Bake;
using detail::baking::INK;
using detail::baking::KeyedImage;
using detail::baking::Keying;
using detail::baking::Painting;
using detail::baking::Pen;
}
