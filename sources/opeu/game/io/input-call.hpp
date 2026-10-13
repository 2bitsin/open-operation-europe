// The calls of the shared input service: the mouse, the menus, the numeric dialog and the keyboard pass one service
// down to each other, so each call has one number across the four modules. Every call takes seven argument words
// (the driver call the register words) unless noted.
#pragma once

#include "common/host.hpp"

#include <cstdint>
#include <utility>

namespace opeu::game::detail::input_call {

// A call with up to eleven argument words.
} // namespace opeu::game::detail::input_call
