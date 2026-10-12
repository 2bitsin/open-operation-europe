// Scrolling the tactical view: clearing a cell off the map, repainting the whole 12 x 12 window, shifting the window by
// one cell (moving what stays on screen and repainting only the exposed row and column), walking the window to a
// target along a line, and the entry that switches the view to a side or scrolls it.
#include "map/mapscrol.hpp"
#include "common/data-segment.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "map/blast.hpp"
#include "map/mapframe.hpp"
#include "map/mapview.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t NO_SIDE{ 255 };
constexpr int16_t VIEW_LAST{ 11 }; // the view shows 12 x 12 cells
constexpr int16_t NO_EDGE{ 255 }; // no exposed column or row on an axis that did not move
constexpr int16_t ORIGIN_MIN{ -4 };
constexpr int16_t ORIGIN_MAX{ 32 };

[[nodiscard]] auto ViewX(DataSegment const& data) -> int16_t {
  return static_cast<int16_t>(data.viewport_x);
}
[[nodiscard]] auto ViewY(DataSegment const& data) -> int16_t {
  return static_cast<int16_t>(data.viewport_y);
}

[[nodiscard]] auto OffMap(int16_t x, int16_t y) -> bool {
  return x < 0 || x >= static_cast<int16_t>(MAP_WIDTH) || y < 0 || y >= static_cast<int16_t>(MAP_HEIGHT);
}

// Clears a cell off the map or draws one on it. frame: the drawing routine's frame.
void PaintCell(uint8_t* ds, HostEvent event, uint16_t frame, int16_t x, int16_t y) {
  if (OffMap(x, y))
    opeu_view_cell_clear(ds, event, x, y);
  else
    opeu_view_cell_draw(ds, event, frame, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
}
} // namespace

// Paints the off-map pattern over one view cell, two pixel rows at a time.
void opeu_view_cell_clear(uint8_t* ds, HostEvent event, int16_t x, int16_t y) {
  auto const& data{ Data(ds) };
  auto const  pixel_x{ static_cast<int16_t>((x - ViewX(data)) * 3 + 4) };
  auto const  pixel_y{ static_cast<int16_t>((y - ViewY(data)) * 24 + 84) };
  for (int16_t row{ 0 }; row < 24; row += 2)
    Call(ds, event, MapService::PATTERN_NEAR, pixel_x, pixel_y + row, 3, 2, 0, Near(data, data.view_clear_pattern));
}

// frame: this routine's frame.
void opeu_view_redraw(uint8_t* ds, HostEvent event, uint16_t frame) {
  auto const& data{ Data(ds) };
  auto const  end_x{ static_cast<int16_t>(ViewX(data) + 12) };
  auto const  end_y{ static_cast<int16_t>(ViewY(data) + 12) };
  for (int16_t y{ ViewY(data) }; end_y > y; y++)
    for (int16_t x{ ViewX(data) }; x < end_x; x++) PaintCell(ds, event, Local(frame, 14), x, y);
}

// Moves the view origin to (x, y), at most one cell on each axis. The cells still on screen move with the screen
// shift, walking from the side that is exposed; the exposed column and row are repainted.
void opeu_view_scroll(uint8_t* ds, HostEvent event, uint16_t frame, int16_t x, int16_t y) {
  auto& data{ Data(ds) };
  if (x == ViewX(data) && y == ViewY(data)) return;
  auto step_x{ static_cast<int16_t>(x - ViewX(data)) };
  auto step_y{ static_cast<int16_t>(y - ViewY(data)) };
  data.viewport_x = static_cast<uint16_t>(x);
  data.viewport_y = static_cast<uint16_t>(y);
  // Walk from the first column/row to the last, starting on the side the view moved toward.
  int16_t first_x{ x };
  int16_t last_x{ static_cast<int16_t>(x + VIEW_LAST) };
  int16_t first_y{ y };
  int16_t last_y{ static_cast<int16_t>(y + VIEW_LAST) };
  if (step_y == -1) std::swap(first_y, last_y);
  int16_t exposed_y{ last_y };
  if (step_x == -1) std::swap(first_x, last_x);
  int16_t    exposed_x{ last_x };
  auto const shift_x{ static_cast<int16_t>(step_x * 3) };
  auto const shift_y{ static_cast<int16_t>(24 * step_y) };
  if (step_x == 0) {
    step_x    = 1;
    exposed_x = NO_EDGE;
  }
  if (step_y == 0) {
    step_y    = 1;
    exposed_y = NO_EDGE;
  }
  for (int16_t cell_y{ first_y };; cell_y += step_y) {
    for (int16_t cell_x{ first_x };; cell_x += step_x) {
      if (cell_x == exposed_x || cell_y == exposed_y) {
        PaintCell(ds, event, Local(frame, 0x22), cell_x, cell_y);
      } else {
        auto const pixel_x{ static_cast<uint16_t>(static_cast<uint16_t>(cell_x - data.viewport_x) * 3u + 4u) };
        auto const pixel_y{ static_cast<uint16_t>(static_cast<uint16_t>(cell_y - data.viewport_y) * 24u + 84u) };
        Call(ds, event, MapService::SCREEN_SHIFT, shift_x + pixel_x, shift_y + pixel_y, pixel_x, pixel_y);
      }
      if (last_x == cell_x) break;
    }
    if (last_y == cell_y) return;
  }
}

// Steps the view origin to (x, y) one cell at a time along a Bresenham line.
void opeu_view_scroll_to(uint8_t* ds, HostEvent event, uint16_t frame, int16_t x, int16_t y) {
  auto const& data{ Data(ds) };
  auto const  delta_x{ static_cast<int16_t>(x - ViewX(data)) };
  auto const  delta_y{ static_cast<int16_t>(y - ViewY(data)) };
  auto const  sign_x{ static_cast<int16_t>(delta_x < 0 ? -1 : 1) };
  auto const  sign_y{ static_cast<int16_t>(delta_y < 0 ? -1 : 1) };
  auto const  span_x{ static_cast<int16_t>(opeu_abs16_bits(static_cast<uint16_t>(delta_x))) };
  auto const  span_y{ static_cast<int16_t>(opeu_abs16_bits(static_cast<uint16_t>(delta_y))) };
  auto const  callee{ Local(frame, 0x1c) };
  if (span_y < span_x) {
    auto error{ static_cast<int16_t>(span_x / 2) };
    auto other{ ViewY(data) };
    for (int16_t along{ ViewX(data) };; along += sign_x) {
      opeu_view_scroll(ds, event, callee, along, other);
      if ((error -= span_y) < 0) {
        error += span_x;
        other += sign_y;
      }
      if (along == x) break;
    }
  } else {
    auto error{ static_cast<int16_t>(span_y / 2) };
    auto other{ ViewX(data) };
    for (int16_t along{ ViewY(data) };; along += sign_y) {
      opeu_view_scroll(ds, event, callee, other, along);
      if ((error -= span_x) < 0) {
        error += span_y;
        other += sign_x;
      }
      if (along == y) break;
    }
  }
}

// For a side in play and an origin within -4..32: switches the tactical view to that side (repainting it all) or
// scrolls it to the origin, then draws the rulers.
void opeu_map_scroll_to(uint8_t* ds, HostEvent event, uint16_t frame, int16_t x, int16_t y, uint8_t side) {
  auto& data{ Data(ds) };
  if (data.side_status[side] == 0 || x < ORIGIN_MIN || x > ORIGIN_MAX || y < ORIGIN_MIN || y > ORIGIN_MAX) return;
  data.overview_side = NO_SIDE;
  Call(ds, event, MapService::STATUS_MODE, 1);
  if (side != data.view_side) {
    data.view_side  = side;
    data.viewport_x = static_cast<uint16_t>(x);
    data.viewport_y = static_cast<uint16_t>(y);
    opeu_view_redraw(ds, event, Local(frame, 12));
  } else if (ViewX(data) != x || ViewY(data) != y) {
    opeu_view_scroll_to(ds, event, Local(frame, 12), x, y);
  }
  opeu_view_frame(ds, event, x, y);
}

uint16_t svc_mapscrol(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count) {
  return svc_map_plan(ds, "mapscrol", kind, args, count);
}
