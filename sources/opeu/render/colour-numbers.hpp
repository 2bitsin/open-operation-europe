#pragma once
// The colour numbers of pixels painted in sixteen known colours: what the bridge to the services' indexed present reads
// back (render-display.hpp RenderIndexed), and what the tests compare.

#include "opeu/assets/image.hpp"
#include "opeu/assets/manifest.hpp"
#include "opeu/assets/texture.hpp"

#include <cstdint>

namespace opeu::render::detail::colour_numbers {

// The colour number of every pixel: the lowest number showing its colour, 0 for a colour no number shows (exact while
// the sixteen colours differ, as the game's palettes do). The image has no mask.
[[nodiscard]] auto NumbersOf(assets::Texture const& pixels, assets::PaletteColours const& colours) -> assets::Image;

}

namespace opeu::render {
using detail::colour_numbers::NumbersOf;
}
