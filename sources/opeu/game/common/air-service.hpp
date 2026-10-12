// The services the air force's routines (core/airbomb, airdrop, airforce, airhelp, airlift) call through their event,
// dispatched by svc_air (core/airbomb). Pointer arguments are DS offsets.
#pragma once

#include <cstdint>

namespace opeu::game::detail::air_service {

enum class AirService : std::uint16_t {
  FORMAT      = 0,  // (destination, format, words...)
  MESSAGE     = 1,  // (text, person)
  CONFIRM     = 2,  // (text, person) -> nonzero when confirmed
  LOOKUP      = 3,  // (message id) -> near pointer to the text
  PRINT       = 4,  // (format, words...) at the army cursor
  SPEAKER     = 5,  // (person, show)
  TEXT_FEED   = 6,  // (text)
  VALUE       = 7,  // (first, last, cancel, items) -> the value chosen
  DIALOG      = 8,  // (text, x, person, menu)
  TITLE       = 9,  // (text)
  PICK        = 10, // (x address, y address) -> nonzero when a cell was picked
  LIST_DIALOG = 11, // (window, limit, flags) -> nonzero when a row was chosen
  POSITION    = 12, // (x, y)
  CONTEXT     = 13, // (side)
  FAR_ACTION  = 14, // (segment, offset) of a mission routine
};
} // namespace opeu::game::detail::air_service

namespace opeu::game {
using detail::air_service::AirService;
} // namespace opeu::game
