#include "opeu/render/colour-numbers.hpp"

#include "opeu/render/rows.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <optional>
#include <ranges>
#include <vector>

namespace opeu::render::detail::colour_numbers {
namespace {

// A colour's red, green and blue bytes as one number, alpha left out.
[[nodiscard]] constexpr auto Packed(std::span<std::byte const> const pixel) noexcept -> std::uint32_t {
  return std::to_integer<std::uint32_t>(pixel[0]) | std::to_integer<std::uint32_t>(pixel[1]) << 8 | std::to_integer<std::uint32_t>(pixel[2]) << 16;
}

[[nodiscard]] constexpr auto Packed(assets::Colour const colour) noexcept -> std::uint32_t {
  return std::uint32_t{ colour.r } | std::uint32_t{ colour.g } << 8 | std::uint32_t{ colour.b } << 16;
}

// The number of each colour, the lowest first; a run of one colour (most of a frame) is answered from the last find.
class NumberFinder {
public:
  explicit NumberFinder(assets::PaletteColours const& colours) {
    std::ranges::transform(colours, _colours.begin(), [](assets::Colour const colour) { return Packed(colour); });
  }

  [[nodiscard]] auto NumberOf(std::uint32_t const colour) -> std::uint8_t {
    if (colour == _last_colour) return _last_number;
    auto const found{ std::ranges::find(_colours, colour) };
    _last_colour = colour;
    _last_number = found == _colours.end() ? std::uint8_t{ 0 } : static_cast<std::uint8_t>(found - _colours.begin());
    return _last_number;
  }

private:
  std::array<std::uint32_t, 16> _colours{};
  std::uint32_t                 _last_colour{ 0xFF000000 }; // no colour packs to it
  std::uint8_t                  _last_number{};
};

[[nodiscard]] auto PixelsOf(assets::Texture const& pixels) {
  return Rows(pixels.Pixels(), RowLayout{ .pitch = 4, .width = 4, .count = pixels.Width() * pixels.Height() });
}

}

auto NumbersOf(assets::Texture const& pixels, assets::PaletteColours const& colours) -> assets::Image {
  auto finder{ NumberFinder{ colours } };
  auto numbers{ PixelsOf(pixels) | std::views::transform([&finder](auto const pixel) { return finder.NumberOf(Packed(pixel)); }) | std::ranges::to<std::vector>() };
  return assets::Image{ pixels.Width(), pixels.Height(), std::move(numbers) };
}

auto Recoloured(assets::Texture const& pixels, assets::PaletteColours const& from, assets::PaletteColours const& to) -> assets::Texture {
  auto       finder{ NumberFinder{ from } };
  auto const recolour{ [&](auto const pixel) {
    auto const colour{ to.at(finder.NumberOf(Packed(pixel))) };
    return std::array{ std::byte{ colour.r }, std::byte{ colour.g }, std::byte{ colour.b }, pixel[3] };
  } };
  return assets::Texture{ pixels.Width(), pixels.Height(), PixelsOf(pixels) | std::views::transform(recolour) | std::views::join | std::ranges::to<std::vector>() };
}

}
