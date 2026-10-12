// The keys the list dialogs act on; the mouse tracking of a list window answers with the same codes.
#pragma once

#include <cstdint>

namespace opeu::game::detail::list_keys {

enum ListKey : std::uint8_t {
  KEY_ENTER       = 0x0d, // choose the selected row (left click on it)
  KEY_ESCAPE      = 0x1b,
  KEY_SCROLL_UP   = 0x1e, // left click on the up arrow
  KEY_SCROLL_DOWN = 0x1f, // left click on the down arrow
  KEY_ZERO        = 0x30, // cancel (right click)
  KEY_DOWN        = 0x32, // '2': the next row (pointer below the selection)
  KEY_UP_5        = 0x35, // '5': the previous row
  KEY_UP          = 0x38, // '8': the previous row (pointer above the selection)
};
} // namespace opeu::game::detail::list_keys

namespace opeu::game {
using namespace detail::list_keys;
} // namespace opeu::game
