// The services only the opening calls: the compositing of its far buffers and the screen (open-compositing.cpp), and
// the FM driver's effect countdown and song bank (open-fm-driver.cpp). Everything else the opening calls in its library
// is the game's and uses the game's service unchanged.
//
// A far argument is the 64 KiB its segment register addresses and an offset in it: the routines use 16-bit offset
// arithmetic, as the original string instructions did (an offset that runs past FFFFh wraps to 0 in the same segment).
#pragma once

#include "machine-display.hpp"
#include "machine-types.hpp"

#include <cstdint>
#include <span>

namespace opeu::services::detail::open_services {
// The bytes a segment register addresses.
using FarSegment = std::span<std::uint8_t, 0x10000>;
// A far address: the segment's bytes and an offset in them.
struct FarBytes {
  FarSegment    segment;
  std::uint16_t offset;
};

// A saved screen rectangle in far memory: `width` columns of 4 bytes (planes 0..3), `height` rows, rows consecutive.
struct SavedRectangle {
  FarBytes      bytes;
  std::uint16_t width;
  std::uint16_t height;
};

// Saves the screen area (8-pixel columns, game lines) into the destination as a saved rectangle.
auto SaveScreenRectangle(Area area, FarBytes destination) -> void;
// Copies count / 2 words from the source to the destination.
auto CopyFarWords(FarBytes source, FarBytes destination, std::uint16_t count) -> void;
// ORs a saved sprite of the same size into a saved rectangle.
auto OrIntoBuffer(SavedRectangle target, FarBytes sprite) -> void;
// ANDs every 8 bytes of a saved rectangle with the mask bytes m0 m1 m0 m1 ...
auto AndWithMask(SavedRectangle target, FarBytes mask) -> void;
// One dissolve step: 288 rows of 304 pixels from the saved picture at `source` to the screen at a column and game line,
// through the 16-bit pixel mask `pattern`, rotated left by one bit per row.
auto DissolveStep(Point at, FarBytes source, std::uint16_t pattern) -> void;

// Starts the FM driver's effect countdown (its counter A) at `ticks`.
auto StartEffectCountdown(std::uint16_t ticks) -> void;
// The effect countdown's ticks left.
auto EffectCountdown() -> std::uint16_t;
// Opens the FM driver with the song bank at the far address; 1 when the driver opened it, 0 when not.
auto OpenSongBank(std::uint16_t offset, std::uint16_t segment) -> std::int16_t;
} // namespace opeu::services::detail::open_services

namespace opeu::services {
using detail::open_services::AndWithMask;
using detail::open_services::CopyFarWords;
using detail::open_services::DissolveStep;
using detail::open_services::EffectCountdown;
using detail::open_services::FarBytes;
using detail::open_services::FarSegment;
using detail::open_services::OpenSongBank;
using detail::open_services::OrIntoBuffer;
using detail::open_services::SavedRectangle;
using detail::open_services::SaveScreenRectangle;
using detail::open_services::StartEffectCountdown;
} // namespace opeu::services
