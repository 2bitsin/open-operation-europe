// List windows (ListWindow in the data segment): the frame, scrolling, the selection bar and mouse tracking.
#pragma once

#include "common/events.hpp"

#include <cstdint>
#include <utility>

namespace opeu::game::detail::list_call {

// The calls of the list service the list windows and the list dialogs share.
enum class ListCall : std::uint16_t {
  DRAW_FAR       = 0, // (x, y, w, h, mode, segment, offset)
  OUTLINE        = 1, // (x1, y1, x2, y2, colour)
  MOUSE_SHOW     = 2,
  MOUSE_HIDE     = 3,
  MOUSE_POSITION = 4, // stores x and y in the first two words
  MOUSE_BUTTONS  = 5, // -> AL
  INPUT_RESET    = 6,
  HIGHLIGHT      = 7,  // (x, y, w, h, mode)
  LINE           = 8,  // (x1, y1, x2, y2, colour)
  SCREEN_COPY    = 9,  // (x, y, w, h, to x, to y)
  BOX            = 10, // (x, y, w, h, colour)
  BITMAP_SAVE    = 11, // (x, y, w, h, segment, offset)
  BITMAP_RESTORE = 12, // (x, y, w, h, mode, segment, offset)
  PAPER          = 13, // (paper)
  TEXT_COLOUR    = 14, // (colour)
  TEXT_POSITION  = 15, // (x, y)
  PRINT          = 16, // (format, seven words)
  KEYBOARD_KEY   = 17, // -> AL
  YES_NO         = 18, // (text) -> AX
  DRAW_ROWS      = 19, // the window's far draw routine (offset, segment, column, row, last column, last row, mode)
  SELECT         = 20, // the window's far select routine (offset, segment, count) -> AL
};

// A call with its argument words.
template <typename... Words>
inline auto List(std::uint8_t* ds, HostWordsEvent event, ListCall call, Words... words) -> std::uint16_t {
  if constexpr (sizeof...(Words) == 0) {
    return event(ds, std::to_underlying(call), nullptr, 0);
  } else {
    std::uint16_t args[]{ static_cast<std::uint16_t>(words)... };
    return event(ds, std::to_underlying(call), args, sizeof...(Words));
  }
}

} // namespace opeu::game::detail::list_call

namespace opeu::game {
using detail::list_call::List;
using detail::list_call::ListCall;
} // namespace opeu::game

void opeu_list_arrows(std::uint8_t* ds, HostWordsEvent event, std::uint16_t x, std::uint16_t top, std::uint16_t bottom,
                      std::uint8_t lit);
void opeu_list_scrollbar(std::uint8_t* ds, HostWordsEvent event, std::uint16_t x, std::uint16_t top,
                         std::uint16_t bottom);
void opeu_table_columns(std::uint8_t* ds, HostWordsEvent event, std::uint8_t column, std::uint8_t rows,
                        std::uint16_t first, std::uint16_t second, std::uint16_t third);
void opeu_list_arrows_lit(std::uint8_t* ds, HostWordsEvent event, std::uint16_t x, std::uint16_t top,
                          std::uint16_t bottom, std::uint8_t lit);
auto opeu_list_mouse_key(std::uint8_t* ds, HostWordsEvent event) -> std::uint8_t;
void opeu_list_invert(std::uint8_t* ds, HostWordsEvent event);
auto opeu_list_frame(std::uint8_t* ds, HostWordsEvent event, std::uint16_t window, std::uint8_t first_column)
    -> std::uint8_t;
void opeu_list_scroll(std::uint8_t* ds, HostWordsEvent event, std::uint16_t window, std::uint8_t column,
                      std::uint8_t row);

// The module's services: the list calls.
auto svc_listui(std::uint8_t* ds, std::uint16_t kind, std::uint16_t* args, std::uint16_t count) -> std::uint16_t;
