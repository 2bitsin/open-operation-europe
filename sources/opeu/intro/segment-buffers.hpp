// The opening's far data: the 64 KiB buffers it allocates and fills, and the screen operations that read or write them
// (the blits, the screen save, the composition of the explosions and the dissolve). A far address is a paragraph of
// OPEN's memory and an offset that wraps inside its 64 KiB, as the original's string instructions did.
#pragma once

#include "open.hpp"

#include <cstdint>

namespace opeu::intro::detail::segment_buffers {

// A place in OPEN's far memory.
struct FarAddress {
  std::uint16_t segment;
  std::uint16_t offset;
};

// A rectangle of the screen in 8-pixel columns and game lines, computed in C's int; each blitter takes it as words.
struct ScreenArea {
  int x;
  int y;
  int width;
  int height;
};

// An image in far memory and the blitter mode it is drawn with.
struct FarImage {
  std::uint16_t segment;
  std::uint16_t offset;
  std::uint16_t mode;
};

// Room for `size` bytes at a far address.
struct FarBuffer {
  std::uint16_t segment;
  std::uint16_t offset;
  std::uint16_t size;
};

// A saved-screen rectangle in far memory: `width` columns of 4 bytes, `height` lines.
struct FarRectangle {
  std::uint16_t segment;
  std::uint16_t offset;
  int           width;
  int           height;
};

// A point of the screen: an 8-pixel column and a game line.
struct ScreenPoint {
  int x;
  int y;
};

// The picture the dissolve reveals and the 16-bit pixel mask of one step.
struct DissolveSource {
  std::uint16_t segment;
  std::uint16_t offset;
  std::uint16_t pattern;
};

// A saved rectangle (4 bytes per column) drawn back at an area of the screen.
auto BlitFar(Open& open, ScreenArea area, FarImage image) -> void;
// A 1-bit image (one byte per column) drawn at an area of the screen.
auto DrawFar(Open& open, ScreenArea area, FarImage image) -> void;
// The screen rectangle saved, 4 bytes per column.
auto SaveScreen(Open& open, ScreenArea area, FarAddress destination) -> void;
// `size` bytes copied a word at a time.
auto CopyFar(Open& open, FarAddress from, FarBuffer to) -> void;
// ORs a sprite into a saved rectangle; ANDs a saved rectangle with a 1-bit mask.
auto OrSprite(Open& open, FarRectangle target, FarAddress sprite) -> void;
auto AndMask(Open& open, FarRectangle target, FarAddress mask) -> void;
// One dissolve step: 288 lines of 38 columns at a point of the screen.
auto Dissolve(Open& open, ScreenPoint at, DissolveSource source) -> void;
// Reads from an open file into far memory; the bytes read, or a DOS error code.
auto ReadFar(Open& open, std::int16_t handle, FarBuffer buffer) -> std::int16_t;
auto FarByte(Open& open, FarAddress at) -> std::uint8_t;
} // namespace opeu::intro::detail::segment_buffers

namespace opeu::intro {
using detail::segment_buffers::AndMask;
using detail::segment_buffers::BlitFar;
using detail::segment_buffers::CopyFar;
using detail::segment_buffers::DissolveSource;
using detail::segment_buffers::Dissolve;
using detail::segment_buffers::DrawFar;
using detail::segment_buffers::FarAddress;
using detail::segment_buffers::FarBuffer;
using detail::segment_buffers::FarByte;
using detail::segment_buffers::FarImage;
using detail::segment_buffers::FarRectangle;
using detail::segment_buffers::OrSprite;
using detail::segment_buffers::ReadFar;
using detail::segment_buffers::SaveScreen;
using detail::segment_buffers::ScreenArea;
using detail::segment_buffers::ScreenPoint;
} // namespace opeu::intro
