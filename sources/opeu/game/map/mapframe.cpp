// Small digit numbers, map scale labels and the ruler frames of the strategic overview and the tactical view.
#include "map/mapframe.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "map/blast.hpp"
#include "types.hpp"
#include "vgalib.hpp"

using namespace opeu::game;

#include <array>
#include <ranges>

namespace {
constexpr uint16_t BLANK{ 10 }; // the glyph after the ten digits
constexpr uint8_t  LAST_CELL{ MAP_WIDTH - 1 };
constexpr uint8_t  LABEL_DIGITS{ 3 };

// A glyph's box on the screen and its colour.
struct GlyphBox {
  ScreenPoint at;
  uint16_t    width;
  uint16_t    height;
  uint16_t    colour;
};

// A ruler line.
struct Segment {
  uint16_t x1;
  uint16_t y1;
  uint16_t x2;
  uint16_t y2;
};

// A scale label: where its last digit (x) or first digit (y) goes, the colour of the blanked cells and of the digits.
struct ScaleLabel {
  ScreenPoint at;
  uint16_t    back;
  uint16_t    fore;
};

// The digit glyph tables: upright 6 x 6 digits, and rotated 8 x 5 ones that run downward.
enum class Glyphs {
  UPRIGHT,
  ROTATED,
};

// A digit glyph table, the glyph size, and the step from one digit to the next.
struct DigitStyle {
  Glyphs   glyphs;
  uint16_t width;
  uint16_t height;
  int16_t  step_x;
  int16_t  step_y;
};
constexpr DigitStyle UPRIGHT{ Glyphs::UPRIGHT, 6, 6, -6, 0 };
constexpr DigitStyle ROTATED{ Glyphs::ROTATED, 8, 5, 0, 5 };

// The near pointer to a digit glyph of the style.
[[nodiscard]] auto DigitGlyph(DataSegment const& data, DigitStyle const& style, uint16_t digit) -> uint16_t {
  if (style.glyphs == Glyphs::UPRIGHT) return static_cast<uint16_t>(Near(data, data.digit_glyphs) + 6 * digit);
  return static_cast<uint16_t>(Near(data, data.rotated_glyphs) + 5 * digit);
}

void DrawGlyph(Game& game, GlyphBox const& box, uint16_t glyph) {
  game.Machine().Display().DrawShiftedBits({ box.at.x, box.at.y, box.width, box.height }, { box.colour, game.State().Bytes(glyph) });
}

void DrawSegment(Game& game, Segment const& segment, uint16_t colour) {
  game.Machine().Display().Line(
      {
          { segment.x1, segment.y1 },
          { segment.x2, segment.y2 }
  },
      colour);
}

// The point `n` digits along from the first digit's.
[[nodiscard]] auto DigitAt(ScreenPoint first, DigitStyle const& style, int n) -> ScreenPoint {
  return { static_cast<uint16_t>(first.x + n * style.step_x), static_cast<uint16_t>(first.y + n * style.step_y) };
}

// The decimal digits of the value, least significant first; 0 has the one digit 0.
[[nodiscard]] auto Digits(uint16_t value) -> std::array<uint16_t, 5> {
  std::array<uint16_t, 5> digits{};
  for (auto& digit : digits) {
    digit  = value % 10;
    value /= 10;
  }
  return digits;
}

[[nodiscard]] auto DigitCount(uint16_t value) -> int {
  int count{ 1 };
  while ((value /= 10) != 0) ++count;
  return count;
}

// Whether a scale label shows: a map distance from 0 to the far edge.
[[nodiscard]] auto OnScale(DataSegment const& data, int16_t value) -> bool {
  return value >= 0 && LAST_CELL * data.map_scale >= value;
}

// Blanks the label's three digit cells in its back colour, then draws the value from the label's point in its fore
// colour when it is on the scale.
void DrawLabel(Game& game, DigitStyle const& style, ScaleLabel const& label, int16_t value) {
  auto const& data{ Data(game) };
  for (auto const n : std::views::iota(0, int{ LABEL_DIGITS })) DrawGlyph(game, { DigitAt(label.at, style, n), style.width, style.height, label.back }, DigitGlyph(data, style, BLANK));
  if (!OnScale(data, value)) return;
  auto const digits{ Digits(static_cast<uint16_t>(value)) };
  for (auto const n : std::views::iota(0, DigitCount(static_cast<uint16_t>(value)))) {
    auto const digit{ digits.at(static_cast<std::size_t>(n)) };
    DrawGlyph(game, { DigitAt(label.at, style, n), style.width, style.height, label.fore }, DigitGlyph(data, style, digit));
  }
}
} // namespace

// Right-aligned, 6 pixels per digit leftward; each digit cell is blanked in colour 0 and drawn in colour 7.
void opeu_view_number(Game& game, ScreenPoint at, uint16_t value) {
  auto const& data{ Data(game) };
  auto const  digits{ Digits(value) };
  for (auto const n : std::views::iota(0, DigitCount(value))) {
    auto const cell{ DigitAt(at, UPRIGHT, n) };
    DrawGlyph(game, { cell, 6, 6, 0 }, DigitGlyph(data, UPRIGHT, BLANK));
    DrawGlyph(game, { cell, 6, 6, 7 }, DigitGlyph(data, UPRIGHT, digits.at(static_cast<std::size_t>(n))));
  }
}

// The overview's rulers in colour 7, labelled 10, 20 and 30 cells times the map scale.
void opeu_overview_frame(Game& game) {
  auto const scale{ Data(game).map_scale };
  DrawSegment(game, { 0x10, 0x18c, 0x150, 0x18c }, 7);
  DrawSegment(game, { 0x10, 0x188, 0x10, 0x18c }, 7);
  DrawSegment(game, { 0x150, 0x188, 0x150, 0x18c }, 7);
  for (auto const tick : std::views::iota(0, 3)) DrawSegment(game, { static_cast<uint16_t>(tick * 0x50 + 0x68), 0x188, static_cast<uint16_t>(tick * 0x50 + 0x68), 0x18c }, 7);
  DrawSegment(game, { 0x15b, 0x44, 0x15b, 0x184 }, 7);
  DrawSegment(game, { 0x157, 0x184, 0x15b, 0x184 }, 7);
  for (auto const tick : std::views::iota(0, 4)) DrawSegment(game, { 0x157, static_cast<uint16_t>(tick * 0x50 + 0x44), 0x15b, static_cast<uint16_t>(tick * 0x50 + 0x44) }, 7);
  DrawLabel(game, UPRIGHT,
            {
                { 0x61, 0x185 },
                8, 7
  },
            static_cast<int16_t>(scale * 10));
  DrawLabel(game, UPRIGHT,
            {
                { 0xb1, 0x185 },
                8, 7
  },
            static_cast<int16_t>(scale * 20));
  DrawLabel(game, UPRIGHT,
            {
                { 0x101, 0x185 },
                8, 7
  },
            static_cast<int16_t>(scale * 30));
  DrawLabel(game, ROTATED,
            {
                { 0x152, 0x97 },
                8, 7
  },
            static_cast<int16_t>(scale * 10));
  DrawLabel(game, ROTATED,
            {
                { 0x152, 0xe7 },
                8, 7
  },
            static_cast<int16_t>(scale * 20));
  DrawLabel(game, ROTATED,
            {
                { 0x152, 0x137 },
                8, 7
  },
            static_cast<int16_t>(scale * 30));
}

// The tactical view's rulers in colour 0, labelled with the scaled map coordinates of view columns and rows x + 3,
// x + 8, y + 3 and y + 8.
void opeu_view_frame(Game& game, int16_t x, int16_t y) {
  auto const scale{ Data(game).map_scale };
  auto const scaled{ [scale](int16_t cell) { return static_cast<int16_t>(static_cast<uint16_t>(cell) * scale); } };
  DrawSegment(game, { 0x20, 0x180, 0x140, 0x180 }, 0);
  for (auto const tick : std::views::iota(0, 13)) DrawSegment(game, { static_cast<uint16_t>(tick * 0x18 + 0x20), 0x17a, static_cast<uint16_t>(tick * 0x18 + 0x20), 0x180 }, 0);
  DrawSegment(game, { 0x14c, 0x54, 0x14c, 0x174 }, 0);
  for (auto const tick : std::views::iota(0, 13)) DrawSegment(game, { 0x148, static_cast<uint16_t>(tick * 0x18 + 0x54), 0x14c, static_cast<uint16_t>(tick * 0x18 + 0x54) }, 0);
  DrawLabel(game, UPRIGHT,
            {
                { 0x77, 0x177 },
                15, 0
  },
            scaled(static_cast<int16_t>(x + 3)));
  DrawLabel(game, UPRIGHT,
            {
                { 0xef, 0x177 },
                15, 0
  },
            scaled(static_cast<int16_t>(x + 8)));
  DrawLabel(game, ROTATED,
            {
                { 0x142, 0xa1 },
                15, 0
  },
            scaled(static_cast<int16_t>(y + 3)));
  DrawLabel(game, ROTATED,
            {
                { 0x142, 0x119 },
                15, 0
  },
            scaled(static_cast<int16_t>(y + 8)));
}
