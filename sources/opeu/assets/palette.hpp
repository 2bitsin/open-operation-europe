#pragma once
// The game's sixteen-colour palettes. No data file holds one: they are the programs' own tables, kept here so the
// pictures can be shown. Each colour is three 4-bit levels in the order blue, red, green; index bit 0 is blue,
// bit 1 red, bit 2 green and bit 3 the brighter half.

#include "opeu/assets/bytes.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace opeu::assets::detail::palette {

struct Colour {
  std::uint8_t r{};
  std::uint8_t g{};
  std::uint8_t b{};

  auto operator==(Colour const&) const -> bool = default;
};

using Levels = std::array<std::uint8_t, 48>;

struct NamedPalette {
  std::string_view name;  // "main/day", "open/005", ...
  Levels levels;
  std::string_view use;   // where the game shows it
};

// The DAC's colour for a 4-bit level (0..15 -> 0..255, as the screen shows it).
[[nodiscard]] constexpr auto Intensity(std::uint8_t const level) noexcept -> std::uint8_t {
  auto const dac{ static_cast<std::uint8_t>(level << 2 & 0x3F) };
  return static_cast<std::uint8_t>(dac << 2 | dac >> 4);
}

[[nodiscard]] constexpr auto Colours(Levels const& levels) noexcept -> std::array<Colour, 16> {
  auto colours{ std::array<Colour, 16>{} };
  std::ranges::transform(Indices(colours.size()), colours.begin(), [&levels](std::size_t const n) {
    return Colour{ .r = Intensity(levels[3 * n + 1]),
                   .g = Intensity(levels[3 * n + 2]),
                   .b = Intensity(levels[3 * n]) };
  });
  return colours;
}

// main/{dawn,day,dusk,night} (the fixed colours 0..7 and the hour's 8..15), main/title, main/map,
// main/epilogue-dark, main/epilogue, and the opening's open/000..open/112.
[[nodiscard]] auto ClassicPalettes() noexcept -> std::span<NamedPalette const>;
[[nodiscard]] auto FindPalette(std::string_view name) -> std::optional<NamedPalette>;

}

namespace opeu::assets {
using detail::palette::ClassicPalettes;
using detail::palette::Colour;
using detail::palette::Colours;
using detail::palette::FindPalette;
using detail::palette::Intensity;
using detail::palette::Levels;
using detail::palette::NamedPalette;
}
