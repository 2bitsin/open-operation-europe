// The initial values of the helpers' DGROUP fields: the picture stream's byte count, the current palette and the black
// palette the fades step between (16 RGB triples of DAC levels 0..15), a palette and a 16x16 one-bit picture no
// routine addresses, and the fatal-error format of the file opener.
#pragma once

#include <array>
#include <cstdint>

namespace opeu::intro::detail::helper_data {

inline constexpr std::uint16_t                STREAM_COUNT{ 0 };
inline constexpr std::array<std::uint8_t, 48> PALETTE{};       // current palette
inline constexpr std::array<std::uint8_t, 48> BLACK_PALETTE{}; // all black

// A palette no routine loads.
inline constexpr std::array<std::uint8_t, 48> SPARE_PALETTE{ 0, 0, 0, 6, 4, 8, 5, 13, 3, 9, 13, 11, 11, 6,  3, 10, 7,  12, 11, 15, 6,  15, 15, 15,
                                                             7, 7, 7, 1, 8, 3, 4, 10, 6, 7, 10, 4,  8,  15, 6, 10, 14, 9,  12, 15, 12, 14, 15, 12 };

// A 16x16 one-bit picture no routine draws, two bytes per row (most significant bit leftmost): a rounded frame
// around an arrow.
inline constexpr std::array<std::array<std::uint8_t, 2>, 16> ICON{
  { { 0x3f, 0xfc },
   { 0x60, 0x06 },
   { 0xc0, 0x03 },
   { 0x80, 0x79 },
   { 0x80, 0x79 },
   { 0x82, 0x79 },
   { 0x86, 0x79 },
   { 0x8f, 0xf9 },
   { 0x9f, 0xf9 },
   { 0xbf, 0xf9 },
   { 0x9f, 0xf1 },
   { 0x8f, 0xe1 },
   { 0x86, 0x01 },
   { 0xc2, 0x03 },
   { 0x60, 0x06 },
   { 0x3f, 0xfc } }
};

inline constexpr std::array<char, 16> ERROR_FORMAT{ std::to_array("OPEN ERROR=  %s") };

} // namespace opeu::intro::detail::helper_data
