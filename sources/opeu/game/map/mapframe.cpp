// Small digit numbers, map scale labels and the ruler frames of the strategic overview and the tactical view.
#include "map/mapframe.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/blast.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
constexpr uint16_t BLANK{ 10 }; // the glyph after the ten digits
constexpr uint8_t  LAST_CELL{ MAP_WIDTH - 1 };

// The near pointer to an upright 6 x 6 digit glyph.
[[nodiscard]] auto UprightGlyph(DataSegment const& data, uint16_t digit) -> uint16_t {
  return static_cast<uint16_t>(Near(data, data.digit_glyphs) + 6 * digit);
}

// The near pointer to a rotated 8 x 5 digit glyph.
[[nodiscard]] auto RotatedGlyph(DataSegment const& data, uint16_t digit) -> uint16_t {
  return static_cast<uint16_t>(Near(data, data.rotated_glyphs) + 5 * digit);
}

void Glyph(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color, uint16_t glyph) {
  Call(ds, event, MapService::GLYPH_NEAR, x, y, w, h, color, glyph);
}

void Line(uint8_t* ds, HostEvent event, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  Call(ds, event, MapService::LINE, x1, y1, x2, y2, color);
}

// Whether a scale label shows: a map distance from 0 to the far edge.
[[nodiscard]] auto OnScale(DataSegment const& data, int16_t value) -> bool {
  return value >= 0 && LAST_CELL * data.map_scale >= value;
}
} // namespace

// A right-aligned unsigned decimal ending at x, 6 pixels per digit leftward; each digit cell is blanked in colour 0
// and drawn in colour 7.
void opeu_view_number(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y, uint16_t value) {
  auto const& data{ Data(ds) };
  for (;; x -= 6) {
    Glyph(ds, event, x, y, 6, 6, 0, UprightGlyph(data, BLANK));
    Glyph(ds, event, x, y, 6, 6, 7, UprightGlyph(data, value % 10));
    if ((value /= 10) == 0) break;
  }
}

// Blanks three upright digit cells leftward from x in colour back, then draws value right-aligned at x in colour fore
// when it is on the scale.
void opeu_scale_label_x(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y, uint16_t back, uint16_t fore, int16_t value) {
  auto const& data{ Data(ds) };
  uint16_t    at{ x };
  for (uint8_t cell{ 0 }; cell < 3; at -= 6, cell++) Glyph(ds, event, at, y, 6, 6, back, UprightGlyph(data, BLANK));
  if (!OnScale(data, value)) return;
  for (;; x -= 6) {
    Glyph(ds, event, x, y, 6, 6, fore, UprightGlyph(data, value % 10));
    if ((value /= 10) == 0) break;
  }
}

// The same with rotated digits running downward from y.
void opeu_scale_label_y(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y, uint16_t back, uint16_t fore, int16_t value) {
  auto const& data{ Data(ds) };
  uint16_t    at{ y };
  for (uint8_t cell{ 0 }; cell < 3; at += 5, cell++) Glyph(ds, event, x, at, 8, 5, back, RotatedGlyph(data, BLANK));
  if (!OnScale(data, value)) return;
  for (;; y += 5) {
    Glyph(ds, event, x, y, 8, 5, fore, RotatedGlyph(data, value % 10));
    if ((value /= 10) == 0) break;
  }
}

// The overview's rulers in colour 7, labelled 10, 20 and 30 cells times the map scale.
void opeu_overview_frame(uint8_t* ds, HostEvent event) {
  auto const scale{ Data(ds).map_scale };
  Line(ds, event, 0x10, 0x18c, 0x150, 0x18c, 7);
  Line(ds, event, 0x10, 0x188, 0x10, 0x18c, 7);
  Line(ds, event, 0x150, 0x188, 0x150, 0x18c, 7);
  for (uint8_t tick{ 0 }; tick < 3; tick++) Line(ds, event, tick * 0x50 + 0x68, 0x188, tick * 0x50 + 0x68, 0x18c, 7);
  Line(ds, event, 0x15b, 0x44, 0x15b, 0x184, 7);
  Line(ds, event, 0x157, 0x184, 0x15b, 0x184, 7);
  for (uint8_t tick{ 0 }; tick < 4; tick++) Line(ds, event, 0x157, tick * 0x50 + 0x44, 0x15b, tick * 0x50 + 0x44, 7);
  opeu_scale_label_x(ds, event, 0x61, 0x185, 8, 7, scale * 10);
  opeu_scale_label_x(ds, event, 0xb1, 0x185, 8, 7, scale * 20);
  opeu_scale_label_x(ds, event, 0x101, 0x185, 8, 7, scale * 30);
  opeu_scale_label_y(ds, event, 0x152, 0x97, 8, 7, scale * 10);
  opeu_scale_label_y(ds, event, 0x152, 0xe7, 8, 7, scale * 20);
  opeu_scale_label_y(ds, event, 0x152, 0x137, 8, 7, scale * 30);
}

// The tactical view's rulers in colour 0, labelled with the scaled map coordinates of view columns and rows x + 3,
// x + 8, y + 3 and y + 8.
void opeu_view_frame(uint8_t* ds, HostEvent event, int16_t x, int16_t y) {
  auto const scale{ Data(ds).map_scale };
  auto const scaled{ [scale](int16_t cell) { return static_cast<int16_t>(static_cast<uint16_t>(cell) * scale); } };
  Line(ds, event, 0x20, 0x180, 0x140, 0x180, 0);
  for (uint8_t tick{ 0 }; tick < 13; tick++) Line(ds, event, tick * 0x18 + 0x20, 0x17a, tick * 0x18 + 0x20, 0x180, 0);
  Line(ds, event, 0x14c, 0x54, 0x14c, 0x174, 0);
  for (uint8_t tick{ 0 }; tick < 13; tick++) Line(ds, event, 0x148, tick * 0x18 + 0x54, 0x14c, tick * 0x18 + 0x54, 0);
  opeu_scale_label_x(ds, event, 0x77, 0x177, 15, 0, scaled(x + 3));
  opeu_scale_label_x(ds, event, 0xef, 0x177, 15, 0, scaled(x + 8));
  opeu_scale_label_y(ds, event, 0x142, 0xa1, 15, 0, scaled(y + 3));
  opeu_scale_label_y(ds, event, 0x142, 0x119, 15, 0, scaled(y + 8));
}

uint16_t svc_mapframe(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count) {
  return svc_map_plan(ds, "mapframe", kind, args, count);
}
