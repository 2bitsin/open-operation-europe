// What the supply panel (core/suppanel, core/supstep) and the transfer panel (units/xferpnl, units/xferstep) share:
// the keypad's geometry, the panel's background, the digit glyphs, the row highlight and the amounts the keys move.
#pragma once

#include "common/data-segment.hpp"
#include "common/memory.hpp"

#include <array>
#include <cstdint>

namespace opeu::game::detail::exchange_panel {

// The keypad: three key columns and three key rows, in pixels from the panel's left edge and top.
inline constexpr std::array<std::uint16_t, 4> KEY_COLUMNS{ 0x48, 0x68, 0x98, 0xb8 };
inline constexpr std::array<std::uint16_t, 4> KEY_ROWS{ 0x23, 0x3f, 0x5b, 0x77 };
inline constexpr std::uint8_t                 NO_KEY{ 0xff };

// The panel's background, saved in the number dialog's image segment.
inline constexpr std::uint16_t PANEL_WIDTH{ 0x20 }; // in 8-pixel columns
inline constexpr std::uint16_t PANEL_HEIGHT{ 0x78 };
inline constexpr std::uint16_t PANEL_IMAGE_AT{ 0x1f00 };

inline constexpr std::int16_t NAME_LIMIT{ 10 }; // characters of a name in the title

// The three resource rows: their pitch in pixels, the highlight's colour and the selection before the first.
inline constexpr std::uint16_t PANEL_ROW_HEIGHT{ 28 };
inline constexpr std::uint16_t HIGHLIGHT_COLOUR{ 0x107 };
inline constexpr std::uint8_t  NO_ROW{ 0xff };

// Digit glyphs are 6 x 6 pixels, 6 bytes each; glyph 10 is blank.
inline constexpr std::uint16_t GLYPH_SIZE{ 6 };
inline constexpr std::uint16_t BLANK_GLYPH{ 10 };
inline constexpr std::uint16_t GLYPH_COLOUR{ 7 };

// The amount a key moves: 1, 10 or 100.
[[nodiscard]] constexpr auto KeyAmount(std::uint8_t key) noexcept -> std::uint16_t {
  switch (key) {
  case '1':
  case '3': return 1;
  case '4':
  case '6': return 10;
  default:  return 100;
  }
}

// Draws `value` right-aligned with its last digit at x through the panel's glyph service, blanking each cell first.
template <typename Event, typename Service>
inline auto DrawDigits(std::uint8_t* ds, Event event, Service glyph, std::uint16_t x, std::uint16_t y, std::uint16_t value) -> void {
  auto const glyphs{ Near(Data(ds), Data(ds).digit_glyphs) };
  for (;; x -= GLYPH_SIZE) {
    Call(ds, event, glyph, x, y, GLYPH_SIZE, GLYPH_SIZE, 0, glyphs + GLYPH_SIZE * BLANK_GLYPH);
    Call(ds, event, glyph, x, y, GLYPH_SIZE, GLYPH_SIZE, GLYPH_COLOUR, glyphs + GLYPH_SIZE * (value % 10));
    if (!(value /= 10)) break;
  }
}

// Toggles the highlight of a row's name on the panel at (panel_x, panel_y) through its highlight service.
template <typename Event, typename Service>
inline auto HighlightRow(std::uint8_t* ds, Event event, Service highlight, std::array<std::uint16_t, 2> panel, std::uint8_t row) -> void {
  Call(ds, event, highlight, panel[0] + 0xd, row * PANEL_ROW_HEIGHT + panel[1] + 0x23, 6, 0x1b, HIGHLIGHT_COLOUR);
}
} // namespace opeu::game::detail::exchange_panel

namespace opeu::game {
using detail::exchange_panel::BLANK_GLYPH;
using detail::exchange_panel::DrawDigits;
using detail::exchange_panel::GLYPH_COLOUR;
using detail::exchange_panel::GLYPH_SIZE;
using detail::exchange_panel::HIGHLIGHT_COLOUR;
using detail::exchange_panel::HighlightRow;
using detail::exchange_panel::KEY_COLUMNS;
using detail::exchange_panel::KEY_ROWS;
using detail::exchange_panel::KeyAmount;
using detail::exchange_panel::NAME_LIMIT;
using detail::exchange_panel::NO_KEY;
using detail::exchange_panel::NO_ROW;
using detail::exchange_panel::PANEL_HEIGHT;
using detail::exchange_panel::PANEL_IMAGE_AT;
using detail::exchange_panel::PANEL_ROW_HEIGHT;
using detail::exchange_panel::PANEL_WIDTH;
} // namespace opeu::game
