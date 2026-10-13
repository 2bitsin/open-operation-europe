// The map inspection mode: the player moves a cursor over the map by keyboard or mouse; the panel shows the objective
// or regiment under it, a click or Enter on one of the side's regiments gives it an order, Escape or the right button
// leaves. The cell is kept in the byte variables at the near addresses x and y.
#include "core/coords.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "map/grid.hpp"
#include "map/objeleg.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "map/viewstep.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/armysum.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/vwpanel.hpp"
#include "io/keyboard.hpp"
#include "io/mouse.hpp"
#include "link/glue.hpp"
#include "map/cursor.hpp"
#include "map/inspect.hpp"
#include "map/maperase.hpp"
#include "map/mappos.hpp"
#include "map/viewpan.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/ordact.hpp"

using namespace opeu::game;

namespace {

// The services the inspection calls through its event.
enum Service : std::uint16_t {
  LOCATION        = 0,  // (x, y, side)
  RESET           = 1,  // () the input
  SAVE            = 2,  // (x, y, width, height, segment, offset) the screen
  ERASE           = 3,  // (x, y) the cursor
  INSPECT         = 4,  // (x, y)
  CURSOR          = 5,  // (x, y)
  OBJECTIVE_PANEL = 6,  // (objective)
  UNIT_PANEL      = 7,  // (unit)
  SUMMARY         = 8,  // () the army summary
  MOUSE_SHOW      = 9,  // ()
  MOUSE_BUTTONS   = 10, // () -> bit 0 left, bit 1 right
  MOUSE_POSITION  = 11, // (&x, &y)
  MOUSE_HIDE      = 12, // ()
  PAN             = 13, // (dx, dy, x, side, y) the tactical view
  UNIT_ACTION     = 14, // (unit, group)
  LOOKUP          = 15, // (message id) -> the text
  MESSAGE         = 16, // (text, person)
  KEY             = 17, // () -> the key
  RESTORE         = 18, // (x, y, width, height, mode, segment, offset) the screen
};

constexpr std::uint8_t  NONE{ 0xff };
constexpr std::uint16_t DIVISION_BUSY{ 100 }; // message id: the division is under the computer's command
constexpr std::uint16_t SAVE_OFFSET{ 0x120 };

// Directions of the tactical view's steps (map/viewstep) a pan stands for.
constexpr std::uint8_t RIGHT{ 4 };
constexpr std::uint8_t LEFT{ 3 };
constexpr std::uint8_t UP{ 6 };
constexpr std::uint8_t DOWN{ 1 };

// The mouse position words, whose addresses go to the mouse service: offsets in the routine's reserve.
constexpr std::uint16_t MOUSE_Y_AT{ 0 };
constexpr std::uint16_t MOUSE_X_AT{ 4 };
constexpr std::uint16_t MOUSE_BYTES{ 6 };

// Mouse areas of the screen.
struct Area {
  std::uint16_t left;
  std::uint16_t top;
  std::uint16_t right;
  std::uint16_t bottom;
};
constexpr Area TACTICAL_MAP{ 32, 84, 319, 371 };
constexpr Area STRATEGIC_MAP{ 16, 68, 335, 387 };

auto Inside(Area const& area, std::uint16_t x, std::uint16_t y) -> bool {
  return x >= area.left && x <= area.right && y >= area.top && y <= area.bottom;
}
} // namespace

// Runs the inspection from the cell at the near addresses x and y; returns 1 when the player picked a cell (or
// ordered a regiment), 0 when he left.
uint16_t opeu_control_coordinates(Game& game, HostEvent event, uint16_t x, uint16_t y) {
  ServiceLocals const locals{ MOUSE_BYTES };
  auto&               data{ Data(game) };
  auto&               cell_x{ Byte(game, x) };
  auto&               cell_y{ Byte(game, y) };
  auto const          mouse_x_at{ locals.At(MOUSE_X_AT) };
  auto const          mouse_y_at{ locals.At(MOUSE_Y_AT) };
  std::uint8_t        previous_x{ cell_x }; // the column shown before; at the start the cursor stays in its column
  auto const          side{ data.current_side };
  auto const          tactical{ [&] { return data.view_side != NONE; } };
  auto const          strategic{ [&] { return data.overview_side != NONE; } };
  auto const          pan{ [&](std::int16_t dx, std::int16_t dy, std::uint8_t at_x, std::uint8_t at_y) { Call(game, event, PAN, dx, dy, at_x, side, at_y); } };
  std::uint16_t       result{ 0 };
  std::uint8_t        display_x{ NONE };
  std::uint8_t        display_y{ 0 };
  std::uint16_t       previous_panel{ 0 };

  // Picks the shown cell: the result is 1. On a regiment the side can order, orders it (or reports that its division
  // is the computer's) and answers true; on a cell of no side or of another regiment, answers false.
  auto const pick{ [&] {
    result = 1;
    auto const owner{ opeu_cell_side(game, display_x, display_y) };
    if (owner == NONE) return false;
    auto const unit{ opeu_find_unit_at(game, display_x, display_y) };
    if (owner != side || !opeu_unit_selection_eligible(game, unit)) return false;
    auto const group{ opeu_unit_group_index(unit) };
    auto const person{ opeu_find_group_record(game, side, group) };
    if (GroupOf(data, side, group).flags & Bits(GroupFlag::AI_MARK))
      Call(game, event, MESSAGE, Call(game, event, LOOKUP, DIVISION_BUSY), person);
    else {
      Call(game, event, UNIT_ACTION, unit, group);
      Call(game, event, ERASE, display_x, display_y);
    }
    return true;
  } };

  cell_x %= MAP_WIDTH;
  cell_y %= MAP_HEIGHT;
  Call(game, event, LOCATION, cell_x, cell_y, side);
  Call(game, event, RESET);
  Call(game, event, SAVE, 0, 0, 80, 24, data.bitmap_buffer, SAVE_OFFSET);
  data.control_selection = 0;
  for (bool done{ false }; !done;) {
    if (display_x != NONE) Call(game, event, ERASE, display_x, display_y);
    display_x = cell_x;
    display_y = cell_y;
    Call(game, event, INSPECT, display_x, display_y);
    Call(game, event, CURSOR, display_x, display_y);
    std::uint16_t panel{ 0 };
    auto          objective{ opeu_find_objective_at(game, display_x, display_y) };
    auto          unit{ opeu_find_unit_at(game, display_x, display_y) };
    // A cell with both: the objective when the cursor came from the left or stayed in the column, else the regiment.
    if (objective && unit) {
      if (previous_x > display_x)
        objective = 0;
      else
        unit = 0;
    }
    if (objective) {
      auto const owner{ opeu_objective_side(game, objective) };
      if ((owner == side && opeu_objective_eligible(game, objective)) || (owner != side && (Record<Objective>(data, objective).flags & Bits(ObjectiveFlag::MARKED)))) {
        Call(game, event, OBJECTIVE_PANEL, objective);
        panel = 1;
      }
    }
    if (unit) {
      auto const owner{ opeu_unit_side(unit) };
      if ((owner == side && opeu_unit_selection_eligible(game, unit)) || (owner != side && (Record<Unit>(data, unit).flags & Bits(UnitFlag::SPOTTED)))) {
        Call(game, event, UNIT_PANEL, unit);
        panel = 1;
      }
    }
    if (!panel && previous_panel == 1) Call(game, event, SUMMARY);
    previous_panel = panel;
    if (data.input_mode) {
      for (;;) {
        Call(game, event, MOUSE_SHOW);
        auto const buttons{ static_cast<std::uint8_t>(Call(game, event, MOUSE_BUTTONS)) };
        Call(game, event, MOUSE_POSITION, mouse_x_at, mouse_y_at);
        auto const mouse_x{ Word(game, mouse_x_at) };
        auto const mouse_y{ Word(game, mouse_y_at) };
        if (buttons & 1) {
          Call(game, event, MOUSE_HIDE);
          if ((tactical() && Inside(TACTICAL_MAP, mouse_x, mouse_y))
              || (strategic() && Inside(STRATEGIC_MAP, mouse_x, mouse_y) && display_x < MAP_WIDTH && display_y < MAP_HEIGHT && opeu_map_code(game, display_x, display_y))) {
            // A click on a cell of no side or of a regiment the side cannot order keeps the inspection going.
            if (!pick()) continue;
            done = true;
            break;
          }
          if (tactical()) {
            std::uint8_t direction{ NONE };
            if (mouse_x >= 320 && mouse_x <= 351 && mouse_y >= 84 && mouse_y <= 371) {
              pan(1, 0, display_x, display_y);
              direction = RIGHT;
            }
            if (mouse_x <= 31 && mouse_y >= 84 && mouse_y <= 371) {
              pan(-1, 0, display_x, display_y);
              direction = LEFT;
            }
            if (mouse_x >= 32 && mouse_x <= 319 && mouse_y >= 56 && mouse_y <= 83) {
              pan(0, -1, display_x, display_y);
              direction = UP;
            }
            if (mouse_x >= 32 && mouse_x <= 319 && mouse_y >= 372 && mouse_y <= 399) {
              pan(0, 1, display_x, display_y);
              direction = DOWN;
            }
            if (direction != NONE) {
              cell_x = display_x;
              cell_y = display_y;
              opeu_grid_step_viewport(game, x, y, direction);
              continue;
            }
          }
        }
        if (buttons & 2) {
          Call(game, event, MOUSE_HIDE);
          result = 0;
          done   = true;
          break;
        }
        if (tactical() && Inside(TACTICAL_MAP, mouse_x, mouse_y)) {
          auto const next_y{ static_cast<std::int16_t>((static_cast<std::int16_t>(mouse_y) - 84) / 24 + static_cast<std::int16_t>(data.viewport_y)) };
          auto const next_x{ static_cast<std::int16_t>((static_cast<std::int16_t>(mouse_x) - 32) / 24 + static_cast<std::int16_t>(data.viewport_x)) };
          if (next_x < 0 || next_y < 0 || next_x >= static_cast<std::int16_t>(MAP_WIDTH) || next_y >= static_cast<std::int16_t>(MAP_HEIGHT)) continue;
          cell_x = static_cast<std::uint8_t>(next_x);
          cell_y = static_cast<std::uint8_t>(next_y);
        } else {
          if (!strategic() || !Inside(STRATEGIC_MAP, mouse_x, mouse_y)) continue;
          cell_x = static_cast<std::uint8_t>((mouse_x - 16) >> 3);
          cell_y = static_cast<std::uint8_t>((mouse_y - 68) >> 3);
        }
        if (cell_x == display_x && cell_y == display_y) continue;
        Call(game, event, MOUSE_HIDE);
        break;
      }
      if (done) break;
    } else {
      std::uint8_t direction{ NONE };
      auto const   key{ static_cast<std::uint8_t>(Call(game, event, KEY)) };
      switch (key) {
      case '6':
        if (tactical()) {
          pan(1, 0, display_x, display_y);
          direction = RIGHT;
        } else
          ++cell_x;
        break;
      case '4':
        if (tactical()) {
          pan(-1, 0, display_x, display_y);
          direction = LEFT;
        } else
          --cell_x;
        break;
      case '8':
        if (tactical()) {
          pan(0, -1, display_x, display_y);
          direction = UP;
        } else
          --cell_y;
        break;
      case '2':
        if (tactical()) {
          pan(0, 1, display_x, display_y);
          direction = DOWN;
        } else
          ++cell_y;
        break;
      case 27:
      case '0':
        result = 0;
        done   = true;
        break;
      case 13:
        if (!opeu_map_code(game, display_x, display_y)) break;
        pick();
        done = true;
        break;
      default:
        for (std::uint8_t i{ 0 }; i < GRID_DIRECTIONS; ++i)
          if (key == static_cast<std::uint8_t>(data.direction_keys[i])) {
            direction = i;
            break;
          }
      }
      if (done) break;
      if (cell_x > MAP_WIDTH) cell_x = 0;
      if (cell_y > MAP_HEIGHT) cell_y = 0;
      if (cell_x > MAP_WIDTH - 1) cell_x = MAP_WIDTH - 1;
      if (cell_y > MAP_HEIGHT - 1) cell_y = MAP_HEIGHT - 1;
      if (tactical())
        opeu_grid_step_viewport(game, x, y, direction);
      else
        opeu_grid_step(&cell_x, &cell_y, direction);
    }
    previous_x = display_x;
  }
  Call(game, event, ERASE, display_x, display_y);
  Call(game, event, RESTORE, 0, 0, 80, 24, 0, data.bitmap_buffer, SAVE_OFFSET);
  return result;
}

uint16_t svc_coords(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
  case LOCATION:
    opeu_arrival_location(game, svc_mappos, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)));
    break;
  case RESET:           opeu_input_reset(game, svc_mouse); break;
  case SAVE:            opeu_bitmap_save(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), port_far(Arg(args, count, 4), Arg(args, count, 5))); break;
  case ERASE:           opeu_map_erase(game, svc_maperase, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1))); break;
  case INSPECT:         opeu_map_inspect(game, svc_inspect, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1))); break;
  case CURSOR:          opeu_map_cursor(game, svc_cursor, Arg(args, count, 0), Arg(args, count, 1)); break;
  case OBJECTIVE_PANEL: opeu_objective_panel(game, svc_vwpanel, Arg(args, count, 0)); break;
  case UNIT_PANEL:      opeu_unit_panel(game, svc_vwpanel, Arg(args, count, 0)); break;
  case SUMMARY:         opeu_army_summary(game, svc_armysum); break;
  case MOUSE_SHOW:      opeu_mouse_show(game, svc_mouse); break;
  case MOUSE_BUTTONS:   result = opeu_mouse_buttons(game, svc_mouse); break;
  case MOUSE_POSITION:  opeu_mouse_position(game, svc_mouse, Arg(args, count, 0), Arg(args, count, 1)); break;
  case MOUSE_HIDE:      opeu_mouse_hide(game, svc_mouse); break;
  case PAN:
    opeu_viewport_pan(game, svc_viewpan, static_cast<std::int16_t>(Arg(args, count, 0)), static_cast<std::int16_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)),
                      static_cast<std::uint8_t>(Arg(args, count, 4)), static_cast<std::uint8_t>(Arg(args, count, 3)));
    break;
  case UNIT_ACTION: opeu_unit_action(game, svc_ordact, Arg(args, count, 0), Arg(args, count, 1)); break;
  case LOOKUP:      result = opeu_message_lookup(game, svc_msglook, static_cast<std::int16_t>(Arg(args, count, 0))); break;
  case MESSAGE:     opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case KEY:         result = opeu_keyboard_key(game, svc_keyboard); break;
  case RESTORE: opeu_bitmap_restore(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), port_far(Arg(args, count, 5), Arg(args, count, 6))); break;
  default:      port_unhandled("coords", id, count);
  }
  return result;
}
