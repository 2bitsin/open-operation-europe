#pragma once
// KOEI's compressed picture stream: a width and height, an optional table of eight run values, then per row the
// commands that fill it in units of four pixels (one nibble in each of the four planes).

#include "opeu/assets/image.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

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
// The length in bytes of the stream that starts at the first byte, header included: DecodePicture's without the
// picture.
[[nodiscard]] auto StreamLength(Bytes bytes, Colours colours) -> Decoded<std::size_t>;

// The next byte of a stream on every call: the game's reader of a file it has open, a test's bytes. It never ends.
using NextByte = std::function<std::uint8_t()>;

// The largest picture a place on the screen holds: a picture whose header states more is refused (BAD_HEADER).
struct Room {
  std::size_t width{ 640 };
  std::size_t height{ 400 };
};

// A picture as its stream draws it: every unit a row's commands write, from the row's first unit, so a row whose
// last command runs past the picture's width shows the units it wrote there; unwritten pixels are masked out.
struct DrawnPicture {
  Image         image;
  std::size_t   width{};       // the header's size
  std::size_t   height{};      //
  std::size_t   length{};      // stream bytes read, header included
  std::uint64_t fingerprint{}; // of those bytes (fingerprint.hpp)
};

// Reads one stream from `next`, header first: BAD_HEADER after the width when it is not 1..640, after the height when
// it is not 1..400 or the picture does not fit `room` (nothing more is read then).
[[nodiscard]] auto DecodePictureAsDrawn(NextByte const& next, Colours colours, Room room) -> Decoded<DrawnPicture>;

}

namespace opeu::assets {
using detail::picture_stream::Colours;
using detail::picture_stream::DecodePicture;
using detail::picture_stream::DecodePictureAsDrawn;
using detail::picture_stream::DrawnPicture;
using detail::picture_stream::NextByte;
using detail::picture_stream::Picture;
using detail::picture_stream::Room;
using detail::picture_stream::StreamLength;
}
