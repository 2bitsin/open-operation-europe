#include "opeu/assets/planar.hpp"

#include <algorithm>
#include <ranges>
#include <tuple>
#include <utility>

namespace opeu::assets::detail::planar {

namespace {

constexpr auto MASK_LAYOUT{ InterleavedLayout(CELL_WIDTH / 8, CELL_HEIGHT, 1) };

// A byte of a layout: its row, byte column and plane.
struct Place {
  std::size_t row;
  std::size_t column;
  std::size_t plane;
};

// Every byte of a layout, row by row, column by column, plane by plane.
[[nodiscard]] auto Places(PlaneLayout const& layout) {
  return Grid(layout.height * layout.width_bytes, layout.planes)
         | std::views::transform([&layout](auto const cell) { return Place{ .row = cell.first / layout.width_bytes, .column = cell.first % layout.width_bytes, .plane = cell.second }; });
}

// Bit `plane` of the eight pixels from `first`, the leftmost in bit 7.
[[nodiscard]] auto Gathered(std::span<std::uint8_t const> const pixels, std::size_t const first, std::size_t const plane) -> std::byte {
  auto const bits{ std::ranges::fold_left(Indices(8), 0U, [&](unsigned const byte, std::size_t const pixel) { return byte << 1 | (pixels[first + pixel] >> plane & 1U); }) };
  return static_cast<std::byte>(bits);
}

}

auto DecodePlanes(Bytes const bytes, PlaneLayout const& layout) -> Decoded<Image> {
  return Need(bytes, 0, layout.Bytes()).transform([&layout](Bytes const image) {
    auto const width{ layout.width_bytes * 8 };
    auto       pixels{ std::vector<std::uint8_t>(width * layout.height) };
    for (auto const [row, column, plane] : Places(layout)) {
      auto const byte{ image[layout.At(plane, column, row)] };
      for (auto const pixel : Indices(8)) pixels[row * width + column * 8 + pixel] |= static_cast<std::uint8_t>(PixelBit(byte, pixel) << plane);
    }
    return Image{ width, layout.height, std::move(pixels) };
  });
}

auto EncodePlanes(Image const& image, PlaneLayout const& layout) -> std::vector<std::byte> {
  auto bytes{ std::vector<std::byte>(layout.Bytes()) };
  for (auto const [row, column, plane] : Places(layout)) bytes[layout.At(plane, column, row)] = Gathered(image.Pixels(), row * image.Width() + column * 8, plane);
  return bytes;
}

auto DecodeInterleaved(Bytes const bytes, std::size_t const width_bytes, std::size_t const height, std::size_t const planes) -> Decoded<Image> {
  return DecodePlanes(bytes, InterleavedLayout(width_bytes, height, planes));
}

auto DecodeBitmap(Bytes const bytes, std::size_t const width_bytes, std::size_t const height) -> Decoded<Image> {
  return DecodeInterleaved(bytes, width_bytes, height, 1);
}

auto DecodeCell(Bytes const bytes) -> Decoded<Image> {
  return Need(bytes, 0, CELL_BYTES).and_then([](Bytes const cell) {
    return DecodePlanes(cell.first(CELL_IMAGE_LAYOUT.Bytes()), CELL_IMAGE_LAYOUT).and_then([cell](Image const& image) {
      return DecodePlanes(cell.subspan(CELL_IMAGE_LAYOUT.Bytes()), MASK_LAYOUT).transform([&image](Image const& keep) {
        auto opaque{ keep.Pixels() | std::views::transform([](std::uint8_t const bit) { return std::uint8_t(bit ^ 1); }) | std::ranges::to<std::vector>() };
        return Image{ image.Width(), image.Height(), image.Pixels() | std::ranges::to<std::vector>(), std::move(opaque) };
      });
    });
  });
}

auto DecodeMaskedSprite(Bytes const image_bytes, Bytes const mask, std::size_t const width_bytes, std::size_t const height) -> Decoded<Image> {
  return DecodeInterleaved(image_bytes, width_bytes, height, 4).and_then([&](Image const& image) {
    return DecodeBitmap(mask, width_bytes, height).transform([&](Image const& shape) {
      auto opaque{ shape.Pixels() | std::views::transform([](std::uint8_t const bit) { return std::uint8_t(bit ^ 1); }) | std::ranges::to<std::vector>() };
      return Image{ image.Width(), image.Height(), image.Pixels() | std::ranges::to<std::vector>(), std::move(opaque) };
    });
  });
}

}
