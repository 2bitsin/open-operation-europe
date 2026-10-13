// The staff selection panel: restoring its bottom and left strips from the back page and marking and framing the
// chosen option.
#include "core/staffpnl.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/uiprim.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace { } // namespace

// Restores the panel's bottom strip.
void opeu_staff_restore_bottom(Game& game) {
  uint16_t x, y;
  for (y = 0x170; y < 0x188; y += 0x10)
    for (x = 0; x < 0x50; x += 2) opeu_screen_copy(0, 0, 2, 0x10, x, y);
}

// Restores the panel's left strip.
void opeu_staff_restore_left(Game& game) {
  uint16_t x, y;
  for (y = 0xf8; y < 0x160; y += 0x10)
    for (x = 0; x < 0x14; x += 2) opeu_screen_copy(0, 0, 2, 0x10, x, y);
}

// Marks an option: options 0..2 sit in the upper row, 3.. in the lower row.
void opeu_staff_mark_option(Game& game, int16_t option) {
  uint16_t start[3];
  uint16_t column;
  start[0] = 0x29;
  start[1] = 0x15;
  start[2] = 0x3d;
  if (option < 3) {
    for (column = start[option]; column < (uint16_t)(start[option] + 10); column += 2) opeu_screen_copy(1, 8, 2, 0x10, column, 0x58);
  } else {
    option -= 3;
    for (column = (uint16_t)(option * 20 + 11); column < (uint16_t)(option * 20 + 0x15); column += 2) opeu_screen_copy(1, 8, 2, 0x10, column, 0x98);
  }
}

// Frames an option and outlines it in colour 7.
void opeu_staff_frame_option(Game& game, int16_t option) {
  uint16_t outline_right[3];
  uint16_t ypos, xpos;
  uint16_t first_col[3];
  uint16_t frame_left[3];
  frame_left[0]    = 0x100;
  frame_left[1]    = 0x60;
  frame_left[2]    = 0x1a0;
  outline_right[0] = 0x13f;
  outline_right[1] = 0x9f;
  outline_right[2] = 0x1df;
  first_col[0]     = 0x20;
  first_col[1]     = 0xc;
  first_col[2]     = 0x34;
  if (option < 3) {
    for (ypos = 0x28; ypos < 0x76; ypos += 0x10)
      for (xpos = first_col[option]; xpos <= (uint16_t)(first_col[option] + 6); xpos += 2) opeu_screen_copy(0, 8, 2, 0x10, xpos, ypos);
    opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), frame_left[option], 0x28, outline_right[option], 0x76, 7);
  } else {
    option -= 3;
    for (ypos = 0x88; ypos < 0xd6; ypos += 0x10)
      for (xpos = (uint16_t)(option * 20 + 2); xpos <= (uint16_t)(option * 20 + 8); xpos += 2) opeu_screen_copy(0, 0, 2, 0x10, xpos, ypos);
    opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), static_cast<uint16_t>(option * 0xa0 + 0x10), 0x88, static_cast<uint16_t>(option * 0xa0 + 0x4f), 0xd6, 7);
  }
}
