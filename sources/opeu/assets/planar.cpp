#include "opeu/assets/planar.hpp"

#include <algorithm>
#include <ranges>
#include <tuple>
#include <utility>

namespace opeu::assets::detail::planar {

namespace {

// The index of pixel `pixel` of a byte column: bit `pixel` of each plane byte, plane 0 the lowest index bit.
[[nodiscard]] auto ColumnIndex(Bytes const column, std::size_t const pixel) -> std::uint8_t {
  return std::ranges::fold_left(std::views::zip(Indices(column.size()), column), std::uint8_t{ 0 },
                                [pixel](std::uint8_t const index, auto const plane_byte) {
                                  return static_cast<std::uint8_t>(
                                      index | PixelBit(std::get<1>(plane_byte), pixel) << std::get<0>(plane_byte));
                                });
}

// The pixels of a row whose plane bytes for each byte column come as `columns`.
template <class Columns>
auto AppendRow(std::vector<std::uint8_t>& pixels, Columns&& columns) -> void {
  for (auto const column : columns) {
    for (auto const pixel : Indices(8)) pixels.push_back(ColumnIndex(column, pixel));
  }
}

}

auto DecodeInterleaved(Bytes const bytes, std::size_t const width_bytes, std::size_t const height,
                       std::size_t const planes) -> Decoded<Image> {
  return Need(bytes, 0, width_bytes * height * planes).transform([&](Bytes const image) {
    auto pixels{ std::vector<std::uint8_t>{} };
    pixels.reserve(width_bytes * 8 * height);
    for (auto const row : Chunks(image, width_bytes * planes)) AppendRow(pixels, Chunks(row, planes));
    return Image{ width_bytes * 8, height, std::move(pixels) };
  });
}

auto DecodeBitmap(Bytes const bytes, std::size_t const width_bytes, std::size_t const height) -> Decoded<Image> {
  return DecodeInterleaved(bytes, width_bytes, height, 1);
}

auto DecodeCell(Bytes const bytes) -> Decoded<Image> {
  constexpr auto PLANE_BYTES{ CELL_WIDTH / 8 };
  constexpr auto IMAGE_BYTES{ CELL_HEIGHT * PLANE_BYTES * 4 };
  return Need(bytes, 0, CELL_BYTES).transform([](Bytes const cell) {
    auto       pixels{ std::vector<std::uint8_t>{} };
    auto       opaque{ std::vector<std::uint8_t>{} };
    auto const rows{ Chunks(cell.first(IMAGE_BYTES), PLANE_BYTES * 4) };
    auto const masks{ Chunks(cell.subspan(IMAGE_BYTES), PLANE_BYTES) };
    for (auto const [row, mask] : std::views::zip(rows, masks)) {
      auto const planes{ Chunks(row, PLANE_BYTES) };
      for (auto const column : Indices(PLANE_BYTES)) {
        auto const column_bytes{ planes | std::views::transform([column](Bytes const plane) { return plane[column]; })
                                 | std::ranges::to<std::vector>() };
        for (auto const pixel : Indices(8)) {
          pixels.push_back(ColumnIndex(column_bytes, pixel));
          opaque.push_back(static_cast<std::uint8_t>(PixelBit(mask[column], pixel) ^ 1));
        }
      }
    }
    return Image{ CELL_WIDTH, CELL_HEIGHT, std::move(pixels), std::move(opaque) };
  });
}

auto DecodeMaskedSprite(Bytes const image_bytes, Bytes const mask, std::size_t const width_bytes,
                        std::size_t const height) -> Decoded<Image> {
  return DecodeInterleaved(image_bytes, width_bytes, height, 4).and_then([&](Image const& image) {
    return DecodeBitmap(mask, width_bytes, height).transform([&](Image const& shape) {
      auto opaque{ shape.Pixels() | std::views::transform([](std::uint8_t const bit) { return std::uint8_t(bit ^ 1); })
                   | std::ranges::to<std::vector>() };
      return Image{ image.Width(), image.Height(), image.Pixels() | std::ranges::to<std::vector>(), std::move(opaque) };
    });
  });
}

}
