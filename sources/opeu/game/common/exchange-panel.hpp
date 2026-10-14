// What the supply panel (core/suppanel, core/supstep) and the transfer panel (units/xferpnl, units/xferstep) share:
// the keypad's geometry, the panel's background, the digit glyphs, the row highlight and the amounts the keys move.
#pragma once

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/screen-point.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace opeu::game::detail::exchange_panel {

// The keypad: three key columns and three key rows, in pixels from the panel's left edge and top.
inline constexpr std::array<std::uint16_t, 4> KEY_COLUMNS{ 0x48, 0x68, 0x98, 0xb8 };
inline constexpr std::array<std::uint16_t, 4> KEY_ROWS{ 0x23, 0x3f, 0x5b, 0x77 };
inline constexpr std::uint8_t                 NO_KEY{ 0xff };

// What the panel's input gives: the key, NO_KEY when a mouse click on the keypad's middle column selected a row.
struct PanelInput {
  std::uint8_t                key;
  std::optional<std::uint8_t> row;
};

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

// The decimal places a 16-bit value can have, units first.
inline constexpr std::array<std::uint16_t, 5> DECIMAL_PLACES{ 1, 10, 100, 1000, 10000 };

// The number of decimal digits of the value; zero has one.
[[nodiscard]] constexpr auto DigitCount(std::uint16_t value) noexcept -> std::size_t {
  return std::max<std::size_t>(1, std::ranges::count_if(DECIMAL_PLACES, [value](std::uint16_t place) { return place <= value; }));
}

// Draws `value` right-aligned with its last digit at `last`, blanking each cell first.
inline auto DrawDigits(Game& game, ScreenPoint last, std::uint16_t value) -> void {
  auto const glyph{ [&](std::uint16_t x, std::uint16_t colour, unsigned digit) {
    game.Machine().Display().DrawShiftedBits({ x, last.y, GLYPH_SIZE, GLYPH_SIZE }, { colour, std::span{ Data(game).digit_glyphs }.subspan(GLYPH_SIZE * digit) });
  } };
  std::ranges::for_each(std::span{ DECIMAL_PLACES }.first(DigitCount(value)), [&, x = last.x](std::uint16_t place) mutable {
    glyph(x, 0, BLANK_GLYPH);
    glyph(x, GLYPH_COLOUR, value / place % 10);
    x = static_cast<std::uint16_t>(x - GLYPH_SIZE);
  });
}

// Toggles the highlight of a row's name on the panel whose top left is `panel`.
inline auto HighlightRow(Game& game, ScreenPoint panel, std::uint8_t row) -> void {
  game.Machine().Display().Highlight({ static_cast<uint16_t>(panel.x + 0xd), static_cast<uint16_t>(row * PANEL_ROW_HEIGHT + panel.y + 0x23), 6, 0x1b }, HIGHLIGHT_COLOUR);
}
} // namespace opeu::game::detail::exchange_panel

namespace opeu::game {
using detail::exchange_panel::BLANK_GLYPH;
using detail::exchange_panel::DECIMAL_PLACES;
using detail::exchange_panel::DigitCount;
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
using detail::exchange_panel::PanelInput;
using detail::exchange_panel::NO_ROW;
using detail::exchange_panel::PANEL_HEIGHT;
using detail::exchange_panel::PANEL_IMAGE_AT;
using detail::exchange_panel::PANEL_ROW_HEIGHT;
using detail::exchange_panel::PANEL_WIDTH;
} // namespace opeu::game
