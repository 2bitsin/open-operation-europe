// The DS stack reserve the map modules' service handlers take for the routines they call, and the cell-to-pixel and
// frame-tile steps the map drawing routines share.
#pragma once

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "glue.hpp"

#include <cstdint>

namespace opeu::game::detail::map_frame {

// Runs body(frame) with the map modules' reserve taken on the DS stack and returns its result; frame is the frame
// pointer of the routine called (entry SP - 2).
template <typename Body>
inline auto WithMapFrame(Body&& body) -> decltype(body(std::uint16_t{})) {
  struct Reserve {
    std::uint16_t frame{ map_frame_enter() };
    ~Reserve() {
      map_frame_leave();
    }
  } const reserve;
  return body(reserve.frame);
}

// A pixel position on the map screen; x counts 8-pixel byte columns.
struct Pixel {
  std::uint16_t x;
  std::uint16_t y;
};

// Converts the cell (x, y) to its pixel position with a map/mappixel converter, which works on coordinate words in
// DS: they are the routine's locals at x_at and y_at.
template <typename Convert>
inline auto CellPixel(std::uint8_t* ds, std::uint16_t x_at, std::uint16_t y_at, std::uint8_t x, std::uint8_t y, Convert convert) -> Pixel {
  SetWord(ds, x_at, x);
  SetWord(ds, y_at, y);
  convert(ds, x_at, y_at);
  return { Word(ds, x_at), Word(ds, y_at) };
}

// The 72-byte cell frame tile n of the frame segment as the far pointer the image stream reads (stream_ptr).
inline auto StreamFrameTile(DataSegment& data, std::uint16_t n) -> FarPointer {
  data.stream_ptr[0] = static_cast<std::uint16_t>(72 * n);
  data.stream_ptr[1] = FRAME_SEGMENT;
  return { data.stream_ptr[0], data.stream_ptr[1] };
}
} // namespace opeu::game::detail::map_frame

namespace opeu::game {
using detail::map_frame::CellPixel;
using detail::map_frame::Pixel;
using detail::map_frame::StreamFrameTile;
using detail::map_frame::WithMapFrame;
} // namespace opeu::game
