// The services the supply transfer between a regiment and its neighbour (units/xferpnl, units/xferstep,
// units/xferunit) calls through its event, and the transfer table's columns.
#pragma once

#include <cstdint>

namespace opeu::game::detail::transfer_service {
enum class TransferService : std::uint16_t {
  MOUSE_SHOW           = 1,
  INPUT_DELAY          = 2, // (ticks)
  MOUSE_POSITION       = 3, // no arguments; x and y come back in args[0] and args[1]
  RIGHT_DOWN           = 4, // -> whether the right button is down
  LEFT_DOWN            = 5, // -> whether the left button is down
  MOUSE_HIDE           = 6,
  KEY                  = 7,  // -> the key pressed
  RESTORE              = 8,  // (x, y, width, height, mode, segment, offset): puts a saved image back
  POSITION             = 9,  // (x, y): the text cursor
  PRINT                = 10, // (format, arguments...)
  GLYPH                = 11, // (x, y, width, height, colour, near glyph)
  TITLE                = 12, // (text): the panel's title
  BOX                  = 14, // (x, y, width, height, colour)
  SOUND_CUE_14         = 15, // the computer's transfer begins
  SOUND_CUE_16         = 16, // the computer's transfer ends
  MUSIC_COMMAND_7      = 17,
  PENDING_FINISH       = 18, // (value)
  DAILY_CONTEXT        = 19, // (side): redraws the side's daily context
  CONFIRM              = 20, // (person) -> whether the player confirms
  INPUT_RESET          = 22,
  HIGHLIGHT            = 23, // (x, y, width, height, colour)
  UNIT_REPORT          = 24, // (unit, text): a report on the regiment to its side
  UNIT_ELIGIBLE        = 25, // (unit) -> whether the regiment may be selected
  DAILY_SHOW           = 26, // (picture, side, offset, segment): the far presenter runs inside the daily picture
  UNIT_CAPACITIES      = 27, // (unit, near ammunition, near fuel, near food): the regiment's capacities
  OBJECTIVE_CAPACITIES = 28, // (objective, near ammunition, near fuel, near food)
  SIDE_PERCENTAGES     = 29, // (side, near ammunition, near fuel, near food, regiments only): the side's fill
  UNIT_SLOT            = 30, // (unit) -> the regiment's place in its division, 0..7
  LAST_SLOT            = 31, // (slot) -> whether it is the division's eighth regiment
  UNIT_GROUP           = 32, // (unit) -> the regiment's division
  UNIT_NAME            = 33, // (unit, buffer) -> the buffer, holding the regiment's name
  UNIT_TRAINING        = 34, // (unit) -> its elements' average training
  UNIT_EXPERIENCE      = 35, // (unit) -> its elements' average experience
  OBJECTIVE_TARGET     = 36, // (near x, near y, side) -> the objective the player picks next to the regiment
  UNIT_TARGET          = 37, // (near x, near y) -> the regiment the player picks next to the current one
  ADJACENT             = 38, // (x, y) -> whether the cell is next to the current regiment
  ORDER_PROGRESS       = 39, // (near order, hours) -> whether the order's countdown reaches 0
  RESTORE_ORDER        = 40, // (unit): brings the regiment's saved order back
};

// The transfer table's columns (DataSegment::transfer_table[side][resource][column]).
enum TransferColumn : std::uint8_t {
  AMOUNT   = 0,
  CAPACITY = 1,
  TARGET   = 2, // what the computer fills the side to
};
inline constexpr std::uint8_t RESOURCES{ 3 }; // ammunition, fuel, food
} // namespace opeu::game::detail::transfer_service

namespace opeu::game {
using detail::transfer_service::AMOUNT;
using detail::transfer_service::CAPACITY;
using detail::transfer_service::RESOURCES;
using detail::transfer_service::TARGET;
using detail::transfer_service::TransferColumn;
using detail::transfer_service::TransferService;
} // namespace opeu::game
