// What a list table's row-draw routine is asked to draw (io/listui.cpp dispatches the routines).
#pragma once

#include <cstdint>

namespace opeu::game::detail::draw_request {

// Columns column..last_column of rows row..last_row, and the area flags (the header, the numbers, the names...: each
// routine's DRAW_* bits).
struct DrawRequest {
  std::uint8_t column;
  std::uint8_t row;
  std::uint8_t last_column;
  std::uint8_t last_row;
  std::uint8_t area;
};
} // namespace opeu::game::detail::draw_request

namespace opeu::game {
using detail::draw_request::DrawRequest;
} // namespace opeu::game
