// The interactive map location picker. With the keyboard, the digit and direction keys move the cursor (or pan the
// tactical view), Enter accepts a passable cell and Esc or '0' cancel. With the mouse, the left button accepts the
// cell under the pointer or pans the tactical view from its borders, and the right button cancels. The cell bytes at x
// and y start at, and end on, the chosen cell; the result is 1 when a cell was accepted.
#include "types.hpp"
#include "map/mappick.hpp"
#include "common/data-segment.hpp"
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

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "common/events.hpp"

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
enum class Outcome { ACCEPT, CANCEL, MOVE };

constexpr uint8_t  NONE{ 255 };
constexpr uint8_t  LEFT_BUTTON{ 1 };
constexpr uint8_t  RIGHT_BUTTON{ 2 };
constexpr uint8_t  KEY_ENTER{ 13 };
constexpr uint8_t  KEY_ESCAPE{ 27 };
constexpr uint16_t STATUS_OFFSET{
  0x120
}; // where the status line's background is saved in the bitmap buffer
// Below the frame: the mouse position words the mouse service writes.
constexpr uint16_t MOUSE_X_AT{ 10 };
constexpr uint16_t MOUSE_Y_AT{ 14 };

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

// frame: this routine's frame. x and y: DS offsets of the cell bytes.
uint16_t opeu_map_pick_location(uint8_t* ds, HostEvent event, uint16_t frame,
                                uint16_t x, uint16_t y) {
  auto&      data{ Data(ds) };
  auto const mouse_x_at{ Local(frame, MOUSE_X_AT) };
  auto const mouse_y_at{ Local(frame, MOUSE_Y_AT) };
  auto const view_up{ [&] { return data.view_side != NONE; } };
  auto const overview_up{ [&] { return data.overview_side != NONE; } };
  if (Byte(ds, y) >= MAP_HEIGHT)
    Byte(ds, y) += 5;
  if (Byte(ds, x) >= MAP_WIDTH)
    Byte(ds, x) += 5;
  Byte(ds, x) %= MAP_WIDTH;
  Byte(ds, y) %= MAP_HEIGHT;
  uint8_t shown_x{ NONE };
  uint8_t shown_y{ 0 };
  if (!data.input_mode)
    Call(ds, event, PickService::ARRIVAL_LOCATION, Byte(ds, x), Byte(ds, y),
         data.current_side);
  Call(ds, event, PickService::INPUT_RESET);
  Call(ds, event, PickService::BITMAP_SAVE, 0, 0, 80, 24, data.bitmap_buffer,
       STATUS_OFFSET);
  data.control_selection = 0;
  auto const pan{ [&](int16_t dx, int16_t dy) {
    Call(ds, event, PickService::VIEWPORT_PAN, dx, dy, shown_x,
         data.current_side, shown_y);
  } };

  // Mouse input until the pointer moves to another cell or a button decides.
  auto const mouse{ [&]() -> Outcome {
    for (;;) {
      Call(ds, event, PickService::MOUSE_SHOW);
      auto const buttons{ static_cast<uint8_t>(
          Call(ds, event, PickService::MOUSE_BUTTONS)) };
      Call(ds, event, PickService::MOUSE_POSITION, mouse_x_at, mouse_y_at);
      // The pointer words are read from DS at each use, as the services in between may write them.
      auto const pointer_x{ [&] { return Word(ds, mouse_x_at); } };
      auto const pointer_y{ [&] { return Word(ds, mouse_y_at); } };
      auto const in{ [&](Area const& area) {
        return area.Holds(pointer_x(), pointer_y());
      } };
      if (buttons & LEFT_BUTTON) {
        Call(ds, event, PickService::MOUSE_HIDE);
        if ((view_up() && in(VIEW_CELLS)) ||
            (overview_up() && in(OVERVIEW_CELLS) && OnMap(shown_x, shown_y) &&
             opeu_map_code(ds, shown_x, shown_y)))
          return Outcome::ACCEPT;
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
            Byte(ds, x) = shown_x;
            Byte(ds, y) = shown_y;
            opeu_grid_step_viewport(ds, x, y, direction);
            Call(ds, event, PickService::MOUSE_HIDE);
            return Outcome::MOVE;
          }
        }
      }
      if (buttons & RIGHT_BUTTON) {
        Call(ds, event, PickService::MOUSE_HIDE);
        return Outcome::CANCEL;
      }
      if (view_up() && in(VIEW_CELLS)) {
        auto const cell_y{ static_cast<int16_t>(
            (static_cast<int16_t>(pointer_y()) - VIEW_CELLS.top) /
                VIEW_CELL_PIXELS +
            static_cast<int16_t>(data.viewport_y)) };
        auto const cell_x{ static_cast<int16_t>(
            (static_cast<int16_t>(pointer_x()) - VIEW_CELLS.left) /
                VIEW_CELL_PIXELS +
            static_cast<int16_t>(data.viewport_x)) };
        if (cell_x < 0 || cell_y < 0 ||
            cell_x >= static_cast<int16_t>(MAP_WIDTH) ||
            cell_y >= static_cast<int16_t>(MAP_HEIGHT))
          continue;
        Byte(ds, x) = static_cast<uint8_t>(cell_x);
        Byte(ds, y) = static_cast<uint8_t>(cell_y);
      } else {
        if (!overview_up() || !in(OVERVIEW_CELLS))
          continue;
        Byte(ds, x) =
            static_cast<uint8_t>((pointer_x() - OVERVIEW_CELLS.left) >> 3);
        Byte(ds, y) =
            static_cast<uint8_t>((pointer_y() - OVERVIEW_CELLS.top) >> 3);
      }
      if (Byte(ds, x) == shown_x && Byte(ds, y) == shown_y)
        continue;
      Call(ds, event, PickService::MOUSE_HIDE);
      return Outcome::MOVE;
    }
  } };

  // One key: a move of the cursor (or a pan of the view), or a decision.
  auto const keyboard{ [&]() -> Outcome {
    uint8_t    direction{ NONE };
    auto const key{ static_cast<uint8_t>(
        Call(ds, event, PickService::KEYBOARD_KEY)) };
    switch (key) {
      case '6':
        if (view_up()) {
          pan(1, 0);
          direction = STEP_RIGHT;
        } else {
          ++Byte(ds, x);
        }
        break;
      case '4':
        if (view_up()) {
          pan(-1, 0);
          direction = STEP_LEFT;
        } else {
          --Byte(ds, x);
        }
        break;
      case '8':
        if (view_up()) {
          pan(0, -1);
          direction = STEP_UP;
        } else {
          --Byte(ds, y);
        }
        break;
      case '2':
        if (view_up()) {
          pan(0, 1);
          direction = STEP_DOWN;
        } else {
          ++Byte(ds, y);
        }
        break;
      case KEY_ESCAPE:
      case '0':        return Outcome::CANCEL;
      case KEY_ENTER:
        if (opeu_map_code(ds, shown_x, shown_y))
          return Outcome::ACCEPT;
        break;
      default:
        for (uint8_t i{ 0 }; i < data.pick_direction_keys.size(); ++i)
          if (key == static_cast<uint8_t>(data.pick_direction_keys[i])) {
            direction = i;
            break;
          }
    }
    // A step off the left or top edge wraps to 255 and lands on 0; off the right or bottom it stops at 39.
    if (Byte(ds, x) > MAP_WIDTH)
      Byte(ds, x) = 0;
    if (Byte(ds, y) > MAP_HEIGHT)
      Byte(ds, y) = 0;
    if (Byte(ds, x) > MAP_WIDTH - 1)
      Byte(ds, x) = MAP_WIDTH - 1;
    if (Byte(ds, y) > MAP_HEIGHT - 1)
      Byte(ds, y) = MAP_HEIGHT - 1;
    if (view_up())
      opeu_grid_step_viewport(ds, x, y, direction);
    else
      opeu_grid_step(ds + x, ds + y, direction);
    return Outcome::MOVE;
  } };

  Outcome outcome;
  do {
    if (shown_x != NONE)
      Call(ds, event, PickService::MAP_ERASE, shown_x, shown_y);
    shown_x = Byte(ds, x);
    shown_y = Byte(ds, y);
    Call(ds, event, PickService::INSPECT, shown_x, shown_y);
    Call(ds, event, PickService::CURSOR, shown_x, shown_y);
    outcome = data.input_mode ? mouse() : keyboard();
  } while (outcome == Outcome::MOVE);
  Call(ds, event, PickService::MAP_ERASE, shown_x, shown_y);
  Call(ds, event, PickService::BITMAP_RESTORE, 0, 0, 80, 24, 0,
       data.bitmap_buffer, STATUS_OFFSET);
  return outcome == Outcome::ACCEPT;
}

uint16_t svc_mappick(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  auto const arg{ [&](unsigned i) { return Arg(args, count, i); } };
  auto const byte{ [&](unsigned i) {
    return static_cast<uint8_t>(Arg(args, count, i));
  } };
  switch (static_cast<PickService>(id)) {
    case PickService::ARRIVAL_LOCATION:
      WithMapFrame([&](uint16_t frame) {
        opeu_arrival_location(ds, svc_mappos, frame, byte(0), byte(1), byte(2));
      });
      return 0;
    case PickService::INPUT_RESET: opeu_input_reset(ds, svc_mouse); return 0;
    case PickService::BITMAP_SAVE:
      opeu_bitmap_save(arg(0), arg(1), arg(2), arg(3),
                       port_far(arg(4), arg(5)));
      return 0;
    case PickService::MAP_ERASE:
      WithMapFrame([&](uint16_t frame) {
        opeu_map_erase(ds, svc_maperase, frame, byte(0), byte(1));
      });
      return 0;
    case PickService::INSPECT:
      opeu_map_inspect(ds, svc_inspect, byte(0), byte(1));
      return 0;
    case PickService::CURSOR:
      WithMapFrame([&](uint16_t frame) {
        opeu_map_cursor(ds, svc_cursor, frame, arg(0), arg(1));
      });
      return 0;
    case PickService::MOUSE_SHOW:    opeu_mouse_show(ds, svc_mouse); return 0;
    case PickService::MOUSE_BUTTONS: return opeu_mouse_buttons(ds, svc_mouse);
    case PickService::MOUSE_POSITION:
      opeu_mouse_position(ds, svc_mouse, arg(0), arg(1));
      return 0;
    case PickService::MOUSE_HIDE: opeu_mouse_hide(ds, svc_mouse); return 0;
    case PickService::VIEWPORT_PAN:
      opeu_viewport_pan(ds, svc_viewpan, static_cast<int16_t>(arg(0)),
                        static_cast<int16_t>(arg(1)), byte(2), byte(4),
                        byte(3));
      return 0;
    case PickService::KEYBOARD_KEY: return opeu_keyboard_key(ds, svc_keyboard);
    case PickService::BITMAP_RESTORE:
      opeu_bitmap_restore(arg(0), arg(1), arg(2), arg(3), arg(4),
                          port_far(arg(5), arg(6)));
      return 0;
    default: port_unhandled("mappick", id, count);
  }
  return 0;
}
