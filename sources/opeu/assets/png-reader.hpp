#pragma once
// Reading a pack's PNG images: the RGBA image as a Texture, the index map as an Image.

#include "opeu/assets/bytes.hpp"
#include "opeu/assets/image.hpp"
#include "opeu/assets/pack-error.hpp"
#include "opeu/assets/texture.hpp"

namespace opeu::assets::detail::png_reader {

// Any PNG as 32-bit pixels R, G, B, A.
[[nodiscard]] auto ReadTexture(Bytes png) -> Packed<Texture>;

inline constexpr auto TRANSPARENT_INDEX{ std::uint8_t{ 255 } };

// An index map (one grey byte per pixel: the palette index, 255 where the background shows) as an indexed image,
// with a mask when some pixel is transparent; a transparent pixel's index is 0.
[[nodiscard]] auto ReadIndexed(Bytes png) -> Packed<Image>;

}

namespace opeu::assets {
using detail::png_reader::ReadIndexed;
using detail::png_reader::ReadTexture;
using detail::png_reader::TRANSPARENT_INDEX;
}
