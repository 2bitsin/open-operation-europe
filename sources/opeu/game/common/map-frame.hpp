// The pixel position and the frame-tile step the map drawing routines share.
#pragma once

#include "common/data-segment.hpp"
#include "common/memory.hpp"

#include <cstdint>

namespace opeu::game::detail::map_frame {

// The 72-byte cell frame tile n of the frame segment as the far pointer the image stream reads (stream_ptr).
inline auto StreamFrameTile(DataSegment& data, std::uint16_t n) -> FarPointer {
  data.stream_ptr[0] = static_cast<std::uint16_t>(72 * n);
  data.stream_ptr[1] = FRAME_SEGMENT;
  return { data.stream_ptr[0], data.stream_ptr[1] };
}
} // namespace opeu::game::detail::map_frame

namespace opeu::game {
using detail::map_frame::StreamFrameTile;
} // namespace opeu::game
