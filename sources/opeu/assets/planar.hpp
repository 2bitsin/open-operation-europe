#pragma once
// The raw bitmap layouts: plane bytes interleaved per byte column, one-bit bitmaps, and the 40x26 cell sprite with
// its AND mask. In every one bit 7 of a byte is the leftmost of its eight pixels and plane p is bit p of the index.
// The same layouts written back (EncodePlanes): the game keeps screen rectangles in its memory in them.

#include "opeu/assets/image.hpp"

#include <compare>
#include <cstddef>
#include <vector>

namespace opeu::assets::detail::planar {

// Where the plane bytes of an image lie: the byte of plane p, byte column c and row r is at
// p * plane_stride + c * column_stride + r * row_stride. Every layout here is dense: planes x width_bytes x height bytes.
struct PlaneLayout {
  std::size_t planes{};
  std::size_t width_bytes{};
  std::size_t height{};
  std::size_t plane_stride{};
  std::size_t column_stride{};
  std::size_t row_stride{};

  [[nodiscard]] constexpr auto Bytes() const noexcept -> std::size_t {
    return planes * width_bytes * height;
  }
  [[nodiscard]] constexpr auto At(std::size_t const plane, std::size_t const column, std::size_t const row) const noexcept -> std::size_t {
    return (plane * plane_stride) + (column * column_stride) + (row * row_stride);
  }

  auto operator<=>(PlaneLayout const&) const = default;
};

// Per row, per byte column, `planes` bytes: the map tiles, dialog bitmaps and portraits, the game's column images.
[[nodiscard]] constexpr auto InterleavedLayout(std::size_t const width_bytes, std::size_t const height, std::size_t const planes) noexcept -> PlaneLayout {
  return PlaneLayout{ .planes = planes, .width_bytes = width_bytes, .height = height, .plane_stride = 1, .column_stride = planes, .row_stride = width_bytes * planes };
}

// Per row, one block of `width_bytes` bytes per plane: the battle cell buffers.
[[nodiscard]] constexpr auto LinePlanesLayout(std::size_t const width_bytes, std::size_t const height, std::size_t const planes) noexcept -> PlaneLayout {
  return PlaneLayout{ .planes = planes, .width_bytes = width_bytes, .height = height, .plane_stride = width_bytes, .column_stride = 1, .row_stride = width_bytes * planes };
}

// One block of rows per plane: the sprite composer's buffer.
[[nodiscard]] constexpr auto PlaneBlocksLayout(std::size_t const width_bytes, std::size_t const height, std::size_t const planes) noexcept -> PlaneLayout {
  return PlaneLayout{ .planes = planes, .width_bytes = width_bytes, .height = height, .plane_stride = width_bytes * height, .column_stride = 1, .row_stride = width_bytes };
}

// The image whose plane bytes lie as `layout` says, width_bytes x 8 pixels wide.
[[nodiscard]] auto DecodePlanes(Bytes bytes, PlaneLayout const& layout) -> Decoded<Image>;

// The plane bytes of `image` (width_bytes x 8 by height pixels at least) laid out as `layout` says; index bits beyond
// the layout's planes are dropped.
[[nodiscard]] auto EncodePlanes(Image const& image, PlaneLayout const& layout) -> std::vector<std::byte>;

// Rows of `width_bytes` bytes, one bit per pixel: a set bit is index 1, a clear bit index 0.
[[nodiscard]] auto DecodeBitmap(Bytes bytes, std::size_t width_bytes, std::size_t height) -> Decoded<Image>;

inline constexpr auto CELL_BYTES{ std::size_t{ 650 } };
inline constexpr auto CELL_WIDTH{ std::size_t{ 40 } };
inline constexpr auto CELL_HEIGHT{ std::size_t{ 26 } };
inline constexpr auto CELL_IMAGE_LAYOUT{ LinePlanesLayout(CELL_WIDTH / 8, CELL_HEIGHT, 4) }; // the cell's first 520 bytes

// The 650-byte cell of the battle sprites: 26 rows of four 5-byte plane rows, then 26 rows of a 5-byte mask whose
// set bits keep the background.
[[nodiscard]] auto DecodeCell(Bytes bytes) -> Decoded<Image>;

// A sprite stored as an interleaved four-plane image and a separate one-bit AND mask (set = background), both
// `width_bytes` byte columns by `height` rows.
struct MaskedSprite {
  Bytes       image;
  Bytes       mask;
  std::size_t width_bytes{};
  std::size_t height{};
};

[[nodiscard]] auto DecodeMaskedSprite(MaskedSprite const& sprite) -> Decoded<Image>;

}

namespace opeu::assets {
using detail::planar::CELL_BYTES;
using detail::planar::CELL_HEIGHT;
using detail::planar::CELL_IMAGE_LAYOUT;
using detail::planar::CELL_WIDTH;
using detail::planar::DecodeBitmap;
using detail::planar::DecodeCell;
using detail::planar::DecodeMaskedSprite;
using detail::planar::DecodePlanes;
using detail::planar::EncodePlanes;
using detail::planar::InterleavedLayout;
using detail::planar::LinePlanesLayout;
using detail::planar::MaskedSprite;
using detail::planar::PlaneBlocksLayout;
using detail::planar::PlaneLayout;
}
