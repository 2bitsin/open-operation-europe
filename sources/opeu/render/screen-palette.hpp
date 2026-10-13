#pragma once
// The colours the game's screen shows: sixteen colour numbers, each mapped to one of 64 palette entries of three 6-bit
// levels, as the game's library sets them (services/vgalib.hpp). The renderer keeps it to know which colour each
// colour number draws in, and the colour number each colour on the frame stands for.

#include "opeu/assets/manifest.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace opeu::render::detail::screen_palette {

inline constexpr auto COLOUR_NUMBERS{ 16uz };
inline constexpr auto PALETTE_ENTRIES{ 256uz };

class ScreenPalette {
public:
  // The start colours: entries 0-63 the 64 colours of two bits per primary, colour 6 showing entry 0x14 (brown) and
  // colours 8-15 entries 0x38-0x3F.
  ScreenPalette();

  // Colours 0-15 show entries 0-15.
  auto ShowIdentity() -> void;
  // Entries from `first & 15`, one per three 4-bit levels (blue, red, green), at most up to entry 15 + first: the
  // count is levels.size() / 3, and a count above 16 becomes 16 - (first & 15).
  auto Load(std::uint8_t first, std::span<std::uint8_t const> levels) -> void;

  // The colour each colour number shows, as the screen's DAC turns 6-bit levels into 8-bit ones.
  [[nodiscard]] auto Colours() const -> assets::PaletteColours;
  // The palette entry colour number `number` shows.
  [[nodiscard]] auto EntryOf(std::size_t number) const -> std::uint8_t {
    return _colour_map.at(number);
  }
  // Every entry's 8-bit levels, red, green, blue (entries past 63 black): the platform's indexed present.
  [[nodiscard]] auto EntryLevels() const -> std::array<std::uint8_t, PALETTE_ENTRIES * 3>;

private:
  struct Entry {
    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
  };

  std::array<Entry, 64>                    _entries{};
  std::array<std::uint8_t, COLOUR_NUMBERS> _colour_map{};
};

// A 6-bit DAC level as the screen shows it in 8 bits.
[[nodiscard]] constexpr auto Shown(std::uint8_t const level) noexcept -> std::uint8_t {
  return static_cast<std::uint8_t>(level << 2 | level >> 4);
}

}

namespace opeu::render {
using detail::screen_palette::COLOUR_NUMBERS;
using detail::screen_palette::PALETTE_ENTRIES;
using detail::screen_palette::ScreenPalette;
using detail::screen_palette::Shown;
}
