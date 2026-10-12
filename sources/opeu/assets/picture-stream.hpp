#pragma once
// KOEI's compressed picture stream: a width and height, an optional table of eight run values, then per row the
// commands that fill it in units of four pixels (one nibble in each of the four planes).

#include "opeu/assets/image.hpp"

#include <cstddef>
#include <cstdint>

namespace opeu::assets::detail::picture_stream {

enum class Colours : std::uint8_t {
  EIGHT,   // literals carry three planes (colours 0..7); no run table
  SIXTEEN, // literals choose among a run table, zero, one or two planes, or all four
};

struct Picture {
  Image       image;
  std::size_t length{}; // stream bytes, header included

  auto operator==(Picture const&) const -> bool = default;
};

// Decodes the stream that starts at the first byte. The picture is decoded as if drawn at the left edge of an
// otherwise black screen, so a copy from a row above the first reads black.
[[nodiscard]] auto DecodePicture(Bytes bytes, Colours colours) -> Decoded<Picture>;

}

namespace opeu::assets {
using detail::picture_stream::Colours;
using detail::picture_stream::DecodePicture;
using detail::picture_stream::Picture;
}
