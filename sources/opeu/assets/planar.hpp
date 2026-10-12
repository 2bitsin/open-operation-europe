#pragma once
// The raw bitmap layouts: plane bytes interleaved per byte column, one-bit bitmaps, and the 40x26 cell sprite with
// its AND mask. In every one bit 7 of a byte is the leftmost of its eight pixels and plane p is bit p of the index.

#include "opeu/assets/image.hpp"

#include <cstddef>

namespace opeu::assets::detail::planar {

// Rows of `width_bytes` byte columns, each column `planes` bytes (plane 0 first): the map tiles, the dialog bitmaps
// (four planes) and the portraits (three planes, colours 0..7).
[[nodiscard]] auto DecodeInterleaved(Bytes bytes, std::size_t width_bytes, std::size_t height, std::size_t planes)
    -> Decoded<Image>;

// Rows of `width_bytes` bytes, one bit per pixel: a set bit is index 1, a clear bit index 0.
[[nodiscard]] auto DecodeBitmap(Bytes bytes, std::size_t width_bytes, std::size_t height) -> Decoded<Image>;

inline constexpr auto CELL_BYTES{ std::size_t{ 650 } };
inline constexpr auto CELL_WIDTH{ std::size_t{ 40 } };
inline constexpr auto CELL_HEIGHT{ std::size_t{ 26 } };

// The 650-byte cell of the battle sprites: 26 rows of four 5-byte plane rows, then 26 rows of a 5-byte mask whose
// set bits keep the background.
[[nodiscard]] auto DecodeCell(Bytes bytes) -> Decoded<Image>;

// A sprite stored as an interleaved four-plane image and a separate one-bit AND mask (set = background).
[[nodiscard]] auto DecodeMaskedSprite(Bytes image, Bytes mask, std::size_t width_bytes, std::size_t height)
    -> Decoded<Image>;

}

namespace opeu::assets {
using detail::planar::CELL_BYTES;
using detail::planar::CELL_HEIGHT;
using detail::planar::CELL_WIDTH;
using detail::planar::DecodeBitmap;
using detail::planar::DecodeCell;
using detail::planar::DecodeInterleaved;
using detail::planar::DecodeMaskedSprite;
}
