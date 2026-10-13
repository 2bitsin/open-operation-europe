#pragma once
// Pixels between the game's colour numbers and the colours on the frame. The game still reads the screen into its
// own memory and computes with colour numbers (an XOR highlight, a sprite composed over the screen); each such call
// reads the frame back and turns its colours into the numbers that show them in the current colours.

#include "opeu/assets/image.hpp"
#include "opeu/assets/manifest.hpp"
#include "opeu/assets/texture.hpp"

#include <cstdint>

namespace opeu::render::detail::colour_numbers {

// The colour number of every pixel: the lowest number showing its colour, 0 for a colour no number shows (exact while
// the sixteen colours differ, as the game's palettes do). The image has no mask.
[[nodiscard]] auto NumbersOf(assets::Texture const& pixels, assets::PaletteColours const& colours) -> assets::Image;

// The pixels in `to` that showed in `from`: the frame after a palette change (alpha kept).
[[nodiscard]] auto Recoloured(assets::Texture const& pixels, assets::PaletteColours const& from, assets::PaletteColours const& to) -> assets::Texture;

}

namespace opeu::render {
using detail::colour_numbers::NumbersOf;
using detail::colour_numbers::Recoloured;
}
