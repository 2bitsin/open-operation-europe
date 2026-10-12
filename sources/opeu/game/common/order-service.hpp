// The services the battle order handlers (units/ordattk, ordbridg, orddefnd, engineer) call through their event,
// dispatched by svc_order_handler (units/engineer). Pointer arguments are DS offsets; byte arguments narrow.
#pragma once

#include <cstdint>

namespace opeu::game::detail::order_service {

enum OrderService : std::uint16_t {
  UNIT_AVERAGE11          = 0,  // (unit)
  RAND_BOUNDED            = 1,  // (bound)
  RESTORE_UNIT_ORDER      = 2,  // (unit)
  GRID_STEP               = 3,  // (&x, &y, direction)
  FIND_UNIT_AT            = 4,  // (x, y)
  UNIT_SIDE               = 5,  // (unit)
  UNIT_VALUE              = 6,  // (unit)
  MOVE_SUPPLIED           = 7,  // ()
  UNIT_REPORT             = 8,  // (unit, text)
  UNIT_SELECTION_ELIGIBLE = 9,  // (unit)
  REQUEST_FORMAT          = 10, // (destination, format, ...)
  CONFIRM_MESSAGE         = 11, // (text, person)
  DAILY_SHOW              = 12, // (id, side, callback offset, callback segment)
  OPPOSITE_SIDE           = 13, // (side)
  MOVE_OBJECTIVE_TARGET   = 14, // (&x, &y, side)
  MOVE_ADJACENT           = 15, // (x, y)
  MOVE_LEVEL_LOWER        = 16, // (record, amount)
  CELL_SIDE               = 17, // (x, y)
  OBJECTIVE_QUANTITY      = 18, // (objective, category, &amount)
  MOVE_UNIT               = 19, // (repeat)
  SIDE_GROUPS_RETARGET    = 20, // (side)
  MOVE_UNIT_TARGET        = 21, // (&x, &y)
  UNIT_NAME               = 22, // (unit, buffer)
  ABS16_BITS              = 23, // (value)
  UNIT_PERSON             = 24, // (unit)
  UNIT_AVERAGE10          = 25, // (unit)
  UNIT_AVERAGE9           = 26, // (unit)
  UNIT_ORDER_DESTINATION  = 27, // (unit, &x, &y)
  MAP_CODE                = 28, // (x, y)
  MAP_MARK5               = 29, // (x, y)
  MAP_ERASE               = 30, // (x, y)
  PENDING_FINISH          = 31, // (value)
  WAIT_UNITS              = 32, // (units)
  SOUND_CUE5              = 33, // ()
};
}

namespace opeu::game {
using detail::order_service::OrderService;
}
