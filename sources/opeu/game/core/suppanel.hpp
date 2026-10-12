// The supply-allocation panel: its keypad, input, table access, title and digits.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

#include <cstdint>

// The services the supply-allocation panel (core/suppanel, core/suprow, core/supstep) calls through its event.
enum class SupplyService : std::uint16_t {
  MOUSE_SHOW     = 1,
  INPUT_DELAY    = 2, // (ticks)
  MOUSE_POSITION = 3, // no arguments; x and y come back in args[0] and args[1]
  RIGHT_DOWN     = 4, // -> whether the right button is down
  LEFT_DOWN      = 5, // -> whether the left button is down
  MOUSE_HIDE     = 6,
  KEY            = 7, // -> the key pressed
  RESTORE =
      8, // (x, y, width, height, mode, segment, offset): puts a saved image back
  POSITION    = 9,  // (x, y): the text cursor
  PRINT       = 10, // (format, arguments...)
  GLYPH       = 11, // (x, y, width, height, colour, near glyph)
  FILL        = 12, // (x, y, width, height, colour)
  ROW         = 20, // (row, column): redraws a row of the panel
  CHIEF_TEXT  = 21, // (text): the Operations chief speaks
  INPUT_RESET = 22,
  HIGHLIGHT   = 23, // (x, y, width, height, colour)
  CHIEF_WAIT =
      24, // (text) -> the Operations chief speaks and waits for the answer
};

uint8_t  opeu_supply_keypad_hit(uint8_t* ds, uint16_t x, uint16_t y);
uint8_t  opeu_supply_input(uint8_t* ds, HostEvent event, uint16_t row);
void     opeu_supply_set(uint8_t* ds, uint8_t side, uint8_t row, uint8_t column,
                         uint16_t value);
uint16_t opeu_supply_get(const uint8_t* ds, uint8_t side, uint8_t row,
                         uint8_t column);
void     opeu_supply_draw(uint8_t* ds, HostEvent event, uint16_t first,
                          uint16_t second);
void opeu_supply_digits(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y,
                        uint16_t value);

// The input, drawing and text services of the panel as direct calls.
uint16_t svc_suppanel(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
