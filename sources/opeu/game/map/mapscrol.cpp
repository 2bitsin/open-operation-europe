// Scrolling the tactical view: clearing a cell off the map, repainting the whole 12 x 12 window, shifting the window by
// one cell (moving what stays on screen and repainting only the exposed row and column), walking the window to a
// target along a line, and the entry that switches the view to a side or scrolls it.
#include "map/mapscrol.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "link/glue.hpp"
#include "map/blast.hpp"
#include "map/mapframe.hpp"
#include "map/mapview.hpp"
#include "syslib-gfx.hpp"
#include "types.hpp"
#include "vgalib.hpp"

using namespace opeu::game;

namespace {
constexpr int16_t VIEW_LAST{ 11 }; // the view shows 12 x 12 cells
constexpr int16_t NO_EDGE{ 255 };  // no exposed column or row on an axis that did not move
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
void PaintCell(Game& game, int16_t x, int16_t y) {
  if (OffMap(x, y))
    opeu_view_cell_clear(game, x, y);
  else
    opeu_view_cell_draw(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
}
} // namespace

// Paints the off-map pattern over one view cell, two pixel rows at a time.
void opeu_view_cell_clear(Game& game, int16_t x, int16_t y) {
  auto const& data{ Data(game) };
  auto const  pixel_x{ static_cast<int16_t>((x - ViewX(data)) * 3 + 4) };
  auto const  pixel_y{ static_cast<int16_t>((y - ViewY(data)) * 24 + 84) };
  for (int16_t row{ 0 }; row < 24; row += 2)
    koei_27c2(static_cast<std::uint16_t>(pixel_x), static_cast<std::uint16_t>(pixel_y + row), 3, 2, 0, game.State().Bytes(static_cast<std::uint16_t>(Near(data, data.view_clear_pattern))).data());
}

// frame: this routine's frame.
void opeu_view_redraw(Game& game) {
  auto const& data{ Data(game) };
  auto const  end_x{ static_cast<int16_t>(ViewX(data) + 12) };
  auto const  end_y{ static_cast<int16_t>(ViewY(data) + 12) };
  for (int16_t y{ ViewY(data) }; end_y > y; y++)
    for (int16_t x{ ViewX(data) }; x < end_x; x++) PaintCell(game, x, y);
}

// Moves the view origin to (x, y), at most one cell on each axis. The cells still on screen move with the screen
// shift, walking from the side that is exposed; the exposed column and row are repainted.
void opeu_view_scroll(Game& game, int16_t x, int16_t y) {
  auto& data{ Data(game) };
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
        PaintCell(game, cell_x, cell_y);
      } else {
        auto const pixel_x{ static_cast<uint16_t>(static_cast<uint16_t>(cell_x - data.viewport_x) * 3u + 4u) };
        auto const pixel_y{ static_cast<uint16_t>(static_cast<uint16_t>(cell_y - data.viewport_y) * 24u + 84u) };
        koei_3bc5(static_cast<std::uint16_t>(shift_x + pixel_x), static_cast<std::uint16_t>(shift_y + pixel_y), static_cast<std::uint16_t>(pixel_x), static_cast<std::uint16_t>(pixel_y));
      }
      if (last_x == cell_x) break;
    }
    if (last_y == cell_y) return;
  }
}

// Steps the view origin to (x, y) one cell at a time along a Bresenham line.
void opeu_view_scroll_to(Game& game, int16_t x, int16_t y) {
  auto const& data{ Data(game) };
  auto const  delta_x{ static_cast<int16_t>(x - ViewX(data)) };
  auto const  delta_y{ static_cast<int16_t>(y - ViewY(data)) };
  auto const  sign_x{ static_cast<int16_t>(delta_x < 0 ? -1 : 1) };
  auto const  sign_y{ static_cast<int16_t>(delta_y < 0 ? -1 : 1) };
  auto const  span_x{ static_cast<int16_t>(opeu_abs16_bits(static_cast<uint16_t>(delta_x))) };
  auto const  span_y{ static_cast<int16_t>(opeu_abs16_bits(static_cast<uint16_t>(delta_y))) };
  if (span_y < span_x) {
    auto error{ static_cast<int16_t>(span_x / 2) };
    auto other{ ViewY(data) };
    for (int16_t along{ ViewX(data) };; along += sign_x) {
      opeu_view_scroll(game, along, other);
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
      opeu_view_scroll(game, other, along);
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
void opeu_map_scroll_to(Game& game, int16_t x, int16_t y, uint8_t side) {
  auto& data{ Data(game) };
  if (data.side_status[side] == SIDE_COMPUTER || x < ORIGIN_MIN || x > ORIGIN_MAX || y < ORIGIN_MIN || y > ORIGIN_MAX) return;
  data.overview_side = NO_SIDE;
  opeu_turn_frame(game, static_cast<DialogHost const*>(port_dialogs_host()), static_cast<uint8_t>(1));
  if (side != data.view_side) {
    data.view_side  = side;
    data.viewport_x = static_cast<uint16_t>(x);
    data.viewport_y = static_cast<uint16_t>(y);
    opeu_view_redraw(game);
  } else if (ViewX(data) != x || ViewY(data) != y) {
    opeu_view_scroll_to(game, x, y);
  }
  opeu_view_frame(game, x, y);
}
