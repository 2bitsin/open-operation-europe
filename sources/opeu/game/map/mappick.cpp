// The interactive map location picker. With the keyboard, the digit and direction keys move the cursor (or pan the
// tactical view), Enter accepts a passable cell and Esc or '0' cancel. With the mouse, the left button accepts the
// cell under the pointer or pans the tactical view from its borders, and the right button cancels. The cursor starts
// on the cell given; the result is the cell accepted, none when cancelled.
#include "map/mappick.hpp"
#include "common/data-segment.hpp"
#include "common/keys.hpp"
#include "common/map-frame.hpp"
#include "common/map-grid.hpp"
#include "io/keyboard.hpp"
#include "io/mouse.hpp"
#include "map/cursor.hpp"
#include "map/grid.hpp"
#include "map/inspect.hpp"
#include "map/maperase.hpp"
#include "map/mappos.hpp"
#include "map/stepmap.hpp"
#include "map/viewpan.hpp"
#include "map/viewstep.hpp"
#include "types.hpp"

#include "common/events.hpp"
#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

#include <optional>
#include <ranges>

using namespace opeu::game;

namespace {
enum class Outcome {
  ACCEPT,
  CANCEL,
  MOVE
};

constexpr uint8_t  NONE{ 255 };
constexpr uint8_t  LEFT_BUTTON{ 1 };
constexpr uint8_t  RIGHT_BUTTON{ 2 };
constexpr uint16_t STATUS_OFFSET{ 0x120 }; // where the status line's background is saved in the bitmap buffer

// Screen areas in pixels (x in pixels here, not byte columns).
struct Area {
  uint16_t                     left, right, top, bottom;
  [[nodiscard]] constexpr auto Holds(uint16_t x, uint16_t y) const -> bool {
    return x >= left && x <= right && y >= top && y <= bottom;
  }
};
constexpr Area    VIEW_CELLS{ 32, 319, 84, 371 };
constexpr Area    OVERVIEW_CELLS{ 16, 335, 68, 387 };
constexpr Area    PAN_RIGHT{ 320, 351, 84, 371 };
constexpr Area    PAN_LEFT{ 0, 31, 84, 371 };
constexpr Area    PAN_UP{ 32, 319, 56, 83 };
constexpr Area    PAN_DOWN{ 32, 319, 372, 399 };
constexpr int16_t VIEW_CELL_PIXELS{ 24 };

// Grid directions (map/step) of the four pans.
constexpr uint8_t STEP_DOWN{ 1 };
constexpr uint8_t STEP_LEFT{ 3 };
constexpr uint8_t STEP_RIGHT{ 4 };
constexpr uint8_t STEP_UP{ 6 };
} // namespace

auto PickLocation(Game& game, GridCell start) -> std::optional<GridCell> {
  auto&      data{ Data(game) };
  auto       cell{ start };
  auto const view_up{ [&] { return data.view_side != NONE; } };
  auto const overview_up{ [&] { return data.overview_side != NONE; } };
  if (cell.y >= MAP_HEIGHT) cell.y += 5;
  if (cell.x >= MAP_WIDTH) cell.x += 5;
  cell.x %= MAP_WIDTH;
  cell.y %= MAP_HEIGHT;
  uint8_t shown_x{ NONE };
  uint8_t shown_y{ 0 };
  if (!data.input_mode) opeu_arrival_location(game, cell.x, cell.y, static_cast<uint8_t>(data.current_side));
  opeu_input_reset(game);
  game.Machine().Display().SaveColumns({ 0, 0, 80, 24 }, FarBytes(game, static_cast<std::uint16_t>(data.bitmap_buffer), STATUS_OFFSET));
  data.control_selection = 0;
  auto const pan{ [&](int16_t dx, int16_t dy) {
    opeu_viewport_pan(game, {
                                .dx = dx, .dy = dy, .cursor = { shown_x, shown_y },
                                        .side = static_cast<uint8_t>(data.current_side)
    });
  } };

  // Mouse input until the pointer moves to another cell or a button decides.
  auto const mouse{ [&]() -> Outcome {
    for (;;) {
      opeu_mouse_show(game);
      auto const buttons{ static_cast<uint8_t>(opeu_mouse_buttons(game)) };
      auto const pointer{ MousePosition(game) };
      auto const in{ [&](Area const& area) { return area.Holds(pointer.x, pointer.y); } };
      if (buttons & LEFT_BUTTON) {
        opeu_mouse_hide(game);
        if ((view_up() && in(VIEW_CELLS)) || (overview_up() && in(OVERVIEW_CELLS) && OnMap(shown_x, shown_y) && opeu_map_code(game, shown_x, shown_y))) return Outcome::ACCEPT;
        if (view_up()) {
          uint8_t direction{ NONE };
          if (in(PAN_RIGHT)) {
            pan(1, 0);
            direction = STEP_RIGHT;
          }
          if (in(PAN_LEFT)) {
            pan(-1, 0);
            direction = STEP_LEFT;
          }
          if (in(PAN_UP)) {
            pan(0, -1);
            direction = STEP_UP;
          }
          if (in(PAN_DOWN)) {
            pan(0, 1);
            direction = STEP_DOWN;
          }
          if (direction != NONE) {
            GridCell const shown{ shown_x, shown_y };
            cell = StepInViewport(data, shown, direction).value_or(shown);
            opeu_mouse_hide(game);
            return Outcome::MOVE;
          }
        }
      }
      if (buttons & RIGHT_BUTTON) {
        opeu_mouse_hide(game);
        return Outcome::CANCEL;
      }
      if (view_up() && in(VIEW_CELLS)) {
        auto const cell_y{ static_cast<int16_t>((static_cast<int16_t>(pointer.y) - VIEW_CELLS.top) / VIEW_CELL_PIXELS + static_cast<int16_t>(data.viewport_y)) };
        auto const cell_x{ static_cast<int16_t>((static_cast<int16_t>(pointer.x) - VIEW_CELLS.left) / VIEW_CELL_PIXELS + static_cast<int16_t>(data.viewport_x)) };
        if (cell_x < 0 || cell_y < 0 || cell_x >= static_cast<int16_t>(MAP_WIDTH) || cell_y >= static_cast<int16_t>(MAP_HEIGHT)) continue;
        cell.x = static_cast<uint8_t>(cell_x);
        cell.y = static_cast<uint8_t>(cell_y);
      } else {
        if (!overview_up() || !in(OVERVIEW_CELLS)) continue;
        cell.x = static_cast<uint8_t>((pointer.x - OVERVIEW_CELLS.left) >> 3);
        cell.y = static_cast<uint8_t>((pointer.y - OVERVIEW_CELLS.top) >> 3);
      }
      if (cell.x == shown_x && cell.y == shown_y) continue;
      opeu_mouse_hide(game);
      return Outcome::MOVE;
    }
  } };

  // One key: a move of the cursor (or a pan of the view), or a decision.
  auto const keyboard{ [&]() -> Outcome {
    uint8_t    direction{ NONE };
    auto const key{ static_cast<uint8_t>(opeu_keyboard_key(game)) };
    switch (key) {
    case '6':
      if (view_up()) {
        pan(1, 0);
        direction = STEP_RIGHT;
      } else {
        ++cell.x;
      }
      break;
    case '4':
      if (view_up()) {
        pan(-1, 0);
        direction = STEP_LEFT;
      } else {
        --cell.x;
      }
      break;
    case '8':
      if (view_up()) {
        pan(0, -1);
        direction = STEP_UP;
      } else {
        --cell.y;
      }
      break;
    case '2':
      if (view_up()) {
        pan(0, 1);
        direction = STEP_DOWN;
      } else {
        ++cell.y;
      }
      break;
    case KEY_ESCAPE:
    case '0':        return Outcome::CANCEL;
    case KEY_ENTER:
      if (opeu_map_code(game, shown_x, shown_y)) return Outcome::ACCEPT;
      break;
    default:
      for (auto const i : std::views::iota(uint8_t{ 0 }, data.pick_direction_keys.size()))
        if (key == static_cast<uint8_t>(data.pick_direction_keys[i])) {
          direction = i;
          break;
        }
    }
    // A step off the left or top edge wraps to 255 and lands on 0; off the right or bottom it stops at 39.
    if (cell.x > MAP_WIDTH) cell.x = 0;
    if (cell.y > MAP_HEIGHT) cell.y = 0;
    if (cell.x > MAP_WIDTH - 1) cell.x = MAP_WIDTH - 1;
    if (cell.y > MAP_HEIGHT - 1) cell.y = MAP_HEIGHT - 1;
    cell = (view_up() ? StepInViewport(data, cell, direction) : StepOnMap(cell, direction)).value_or(cell);
    return Outcome::MOVE;
  } };

  Outcome outcome;
  do {
    if (shown_x != NONE) opeu_map_erase(game, static_cast<uint8_t>(shown_x), static_cast<uint8_t>(shown_y));
    shown_x = cell.x;
    shown_y = cell.y;
    opeu_map_inspect(game, static_cast<uint8_t>(shown_x), static_cast<uint8_t>(shown_y));
    opeu_map_cursor(game, shown_x, shown_y);
    outcome = data.input_mode ? mouse() : keyboard();
  } while (outcome == Outcome::MOVE);
  opeu_map_erase(game, static_cast<uint8_t>(shown_x), static_cast<uint8_t>(shown_y));
  game.Machine().Display().DrawColumns({ 0, 0, 80, 24 }, { 0, FarBytes(game, static_cast<std::uint16_t>(data.bitmap_buffer), STATUS_OFFSET) });
  if (outcome != Outcome::ACCEPT) return std::nullopt;
  return cell;
}
