// The calls of the shared input service: the mouse, the menus, the numeric dialog and the keyboard pass one service
// down to each other, so each call has one number across the four modules. Every call takes seven argument words
// (the driver call the register words) unless noted.
#pragma once

#include "common/events.hpp"
#include "common/host.hpp"

#include <cstdint>
#include <utility>

namespace opeu::game::detail::input_call {

enum class InputCall : std::uint16_t {
  MOUSE_INT86    = INT86_CALL, // int86(0x33)
  TEXT_SAVE      = 2,
  TEXT_POSITION  = 3, // (x, y)
  TEXT_PRINT     = 4, // (format, character, text)
  TEXT_RESTORE   = 5,
  TILE_BLIT      = 6,  // (x, y, w, h, segment, offset)
  BITMAP_SAVE    = 7,  // (x, y, w, h, segment, offset)
  BITMAP_RESTORE = 8,  // (x, y, w, h, mode, segment, offset)
  DELAY_UNITS    = 9,  // (units)
  UI_FILL        = 10, // (x1, y1, x2, y2, colour)
  UI_INVERT      = 11, // (x, y, w, h, mode)
  UI_LINE        = 13, // (x1, y1, x2, y2, colour)
  READ_KEY       = 20, // -> key
  INPUT_FLUSH    = 21,
  TEXT_HOME      = 22, // (y)
  TEXT_PUT       = 23, // (character)
  TEXT_BACKSPACE = 24,
  TIMER_START    = 25, // (ticks)
  TIMER_RUNNING  = 26, // -> word
  KEY_PENDING    = 27, // -> word
  SPRITE_DRAW    = 28, // (x, y, w, h, image offset, segment, mask offset, segment, buffer offset, segment, flags)
  DRAW_FAR       = 29, // (x, y, w, h, mode, segment, offset)
  CURSOR_INVERT  = 30, // (x, y, w, h, mode)
  DELAY_TICKS    = 31, // (ticks)
  PENDING_FINISH = 32, // (value)
  PRINT          = 33, // (format, text): two words
  IDLE_HOOK      = 34, // (offset, segment): a far routine
};

// A call with up to eleven argument words.
template <typename... Words>
inline auto Input(Game& game, HostService service, InputCall call, Words... words) -> std::uint16_t {
  std::uint16_t args[sizeof...(Words) < 7 ? 7 : sizeof...(Words)]{ static_cast<std::uint16_t>(words)... };
  return service(game, std::to_underlying(call), args);
}
} // namespace opeu::game::detail::input_call

namespace opeu::game {
using detail::input_call::Input;
using detail::input_call::InputCall;
} // namespace opeu::game
