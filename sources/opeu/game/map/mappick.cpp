// The interactive map location picker. With the keyboard, the digit and direction keys move the cursor (or pan the
// tactical view), Enter accepts a passable cell and Esc or '0' cancel. With the mouse, the left button accepts the
// cell under the pointer or pans the tactical view from its borders, and the right button cancels. The cell bytes at x
// and y start at, and end on, the chosen cell; the result is 1 when a cell was accepted.
#include "map/mappick.hpp"
#include "common/data-segment.hpp"
#include "common/keys.hpp"
#include "common/map-frame.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
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
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
enum class PickService : uint16_t {
  ARRIVAL_LOCATION = 0,  // x, y, side
  INPUT_RESET      = 1,  //
  BITMAP_SAVE      = 2,  // x, y, w, h, segment, offset
  MAP_ERASE        = 3,  // x, y
  INSPECT          = 4,  // x, y
  CURSOR           = 5,  // x, y
  MOUSE_SHOW       = 9,  //
  MOUSE_BUTTONS    = 10, //
  MOUSE_POSITION   = 11, // DS offsets of the x and y words it writes
  MOUSE_HIDE       = 12, //
  VIEWPORT_PAN     = 13, // dx, dy, x, side, y
  KEYBOARD_KEY     = 17, //
  BITMAP_RESTORE   = 18, // x, y, w, h, mode, segment, offset
};
enum class Outcome {
  ACCEPT,
  CANCEL,
  MOVE
};

constexpr uint8_t  NONE{ 255 };
constexpr uint8_t  LEFT_BUTTON{ 1 };
constexpr uint8_t  RIGHT_BUTTON{ 2 };
constexpr uint16_t STATUS_OFFSET{ 0x120 }; // where the status line's background is saved in the bitmap buffer
// The DS reserve of the mouse position words the mouse service writes.
constexpr uint16_t MOUSE_RESERVE{ 14 };
constexpr uint16_t MOUSE_X_AT{ 4 };
constexpr uint16_t MOUSE_Y_AT{ 0 };

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

// x and y: DS offsets of the cell bytes.
uint16_t opeu_map_pick_location(Game& game, HostEvent event, uint16_t x, uint16_t y) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ MOUSE_RESERVE };
  auto const          mouse_x_at{ locals.At(MOUSE_X_AT) };
  auto const          mouse_y_at{ locals.At(MOUSE_Y_AT) };
  auto const          view_up{ [&] { return data.view_side != NONE; } };
  auto const          overview_up{ [&] { return data.overview_side != NONE; } };
  if (Byte(game, y) >= MAP_HEIGHT) Byte(game, y) += 5;
  if (Byte(game, x) >= MAP_WIDTH) Byte(game, x) += 5;
  Byte(game, x) %= MAP_WIDTH;
  Byte(game, y) %= MAP_HEIGHT;
  uint8_t shown_x{ NONE };
  uint8_t shown_y{ 0 };
  if (!data.input_mode) Call(game, event, PickService::ARRIVAL_LOCATION, Byte(game, x), Byte(game, y), data.current_side);
  Call(game, event, PickService::INPUT_RESET);
  Call(game, event, PickService::BITMAP_SAVE, 0, 0, 80, 24, data.bitmap_buffer, STATUS_OFFSET);
  data.control_selection = 0;
  auto const pan{ [&](int16_t dx, int16_t dy) { Call(game, event, PickService::VIEWPORT_PAN, dx, dy, shown_x, data.current_side, shown_y); } };

  // Mouse input until the pointer moves to another cell or a button decides.
  auto const mouse{ [&]() -> Outcome {
    for (;;) {
      Call(game, event, PickService::MOUSE_SHOW);
      auto const buttons{ static_cast<uint8_t>(Call(game, event, PickService::MOUSE_BUTTONS)) };
      Call(game, event, PickService::MOUSE_POSITION, mouse_x_at, mouse_y_at);
      // The pointer words are read from DS at each use, as the services in between may write them.
      auto const pointer_x{ [&] { return Word(game, mouse_x_at); } };
      auto const pointer_y{ [&] { return Word(game, mouse_y_at); } };
      auto const in{ [&](Area const& area) { return area.Holds(pointer_x(), pointer_y()); } };
      if (buttons & LEFT_BUTTON) {
        Call(game, event, PickService::MOUSE_HIDE);
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
            Byte(game, x) = shown_x;
            Byte(game, y) = shown_y;
            opeu_grid_step_viewport(game, x, y, direction);
            Call(game, event, PickService::MOUSE_HIDE);
            return Outcome::MOVE;
          }
        }
      }
      if (buttons & RIGHT_BUTTON) {
        Call(game, event, PickService::MOUSE_HIDE);
        return Outcome::CANCEL;
      }
      if (view_up() && in(VIEW_CELLS)) {
        auto const cell_y{ static_cast<int16_t>((static_cast<int16_t>(pointer_y()) - VIEW_CELLS.top) / VIEW_CELL_PIXELS + static_cast<int16_t>(data.viewport_y)) };
        auto const cell_x{ static_cast<int16_t>((static_cast<int16_t>(pointer_x()) - VIEW_CELLS.left) / VIEW_CELL_PIXELS + static_cast<int16_t>(data.viewport_x)) };
        if (cell_x < 0 || cell_y < 0 || cell_x >= static_cast<int16_t>(MAP_WIDTH) || cell_y >= static_cast<int16_t>(MAP_HEIGHT)) continue;
        Byte(game, x) = static_cast<uint8_t>(cell_x);
        Byte(game, y) = static_cast<uint8_t>(cell_y);
      } else {
        if (!overview_up() || !in(OVERVIEW_CELLS)) continue;
        Byte(game, x) = static_cast<uint8_t>((pointer_x() - OVERVIEW_CELLS.left) >> 3);
        Byte(game, y) = static_cast<uint8_t>((pointer_y() - OVERVIEW_CELLS.top) >> 3);
      }
      if (Byte(game, x) == shown_x && Byte(game, y) == shown_y) continue;
      Call(game, event, PickService::MOUSE_HIDE);
      return Outcome::MOVE;
    }
  } };

  // One key: a move of the cursor (or a pan of the view), or a decision.
  auto const keyboard{ [&]() -> Outcome {
    uint8_t    direction{ NONE };
    auto const key{ static_cast<uint8_t>(Call(game, event, PickService::KEYBOARD_KEY)) };
    switch (key) {
    case '6':
      if (view_up()) {
        pan(1, 0);
        direction = STEP_RIGHT;
      } else {
        ++Byte(game, x);
      }
      break;
    case '4':
      if (view_up()) {
        pan(-1, 0);
        direction = STEP_LEFT;
      } else {
        --Byte(game, x);
      }
      break;
    case '8':
      if (view_up()) {
        pan(0, -1);
        direction = STEP_UP;
      } else {
        --Byte(game, y);
      }
      break;
    case '2':
      if (view_up()) {
        pan(0, 1);
        direction = STEP_DOWN;
      } else {
        ++Byte(game, y);
      }
      break;
    case KEY_ESCAPE:
    case '0':        return Outcome::CANCEL;
    case KEY_ENTER:
      if (opeu_map_code(game, shown_x, shown_y)) return Outcome::ACCEPT;
      break;
    default:
      for (uint8_t i{ 0 }; i < data.pick_direction_keys.size(); ++i)
        if (key == static_cast<uint8_t>(data.pick_direction_keys[i])) {
          direction = i;
          break;
        }
    }
    // A step off the left or top edge wraps to 255 and lands on 0; off the right or bottom it stops at 39.
    if (Byte(game, x) > MAP_WIDTH) Byte(game, x) = 0;
    if (Byte(game, y) > MAP_HEIGHT) Byte(game, y) = 0;
    if (Byte(game, x) > MAP_WIDTH - 1) Byte(game, x) = MAP_WIDTH - 1;
    if (Byte(game, y) > MAP_HEIGHT - 1) Byte(game, y) = MAP_HEIGHT - 1;
    if (view_up())
      opeu_grid_step_viewport(game, x, y, direction);
    else
      opeu_grid_step(game.State().Bytes(x, 1).data(), game.State().Bytes(y, 1).data(), direction);
    return Outcome::MOVE;
  } };

  Outcome outcome;
  do {
    if (shown_x != NONE) Call(game, event, PickService::MAP_ERASE, shown_x, shown_y);
    shown_x = Byte(game, x);
    shown_y = Byte(game, y);
    Call(game, event, PickService::INSPECT, shown_x, shown_y);
    Call(game, event, PickService::CURSOR, shown_x, shown_y);
    outcome = data.input_mode ? mouse() : keyboard();
  } while (outcome == Outcome::MOVE);
  Call(game, event, PickService::MAP_ERASE, shown_x, shown_y);
  Call(game, event, PickService::BITMAP_RESTORE, 0, 0, 80, 24, 0, data.bitmap_buffer, STATUS_OFFSET);
  return outcome == Outcome::ACCEPT;
}

uint16_t svc_mappick(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  auto const arg{ [&](unsigned i) { return Arg(args, count, i); } };
  auto const byte{ [&](unsigned i) { return static_cast<uint8_t>(Arg(args, count, i)); } };
  switch (static_cast<PickService>(id)) {
  case PickService::ARRIVAL_LOCATION: opeu_arrival_location(game, svc_mappos, byte(0), byte(1), byte(2)); return 0;
  case PickService::INPUT_RESET:      opeu_input_reset(game, svc_mouse); return 0;
  case PickService::BITMAP_SAVE:      opeu_bitmap_save(arg(0), arg(1), arg(2), arg(3), port_far(arg(4), arg(5))); return 0;
  case PickService::MAP_ERASE:        opeu_map_erase(game, svc_maperase, byte(0), byte(1)); return 0;
  case PickService::INSPECT:          opeu_map_inspect(game, svc_inspect, byte(0), byte(1)); return 0;
  case PickService::CURSOR:           opeu_map_cursor(game, svc_cursor, arg(0), arg(1)); return 0;
  case PickService::MOUSE_SHOW:       opeu_mouse_show(game, svc_mouse); return 0;
  case PickService::MOUSE_BUTTONS:    return opeu_mouse_buttons(game, svc_mouse);
  case PickService::MOUSE_POSITION:   opeu_mouse_position(game, svc_mouse, arg(0), arg(1)); return 0;
  case PickService::MOUSE_HIDE:       opeu_mouse_hide(game, svc_mouse); return 0;
  case PickService::VIEWPORT_PAN:     opeu_viewport_pan(game, svc_viewpan, static_cast<int16_t>(arg(0)), static_cast<int16_t>(arg(1)), byte(2), byte(4), byte(3)); return 0;
  case PickService::KEYBOARD_KEY:     return opeu_keyboard_key(game, svc_keyboard);
  case PickService::BITMAP_RESTORE:   opeu_bitmap_restore(arg(0), arg(1), arg(2), arg(3), arg(4), port_far(arg(5), arg(6))); return 0;
  default:                            port_unhandled("mappick", id, count);
  }
  return 0;
}
