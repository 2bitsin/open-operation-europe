// The far segments of MAIN's data, as the game files address them.
#pragma once

#include <cstdint>

namespace opeu::game::detail::memory {

// Paragraphs of the far data the game addresses as segment:offset pairs (unrelocated, as the services expect them).
inline constexpr std::uint16_t DATA_SEGMENT{ 0x4712 };       // DS (= SS): a far pointer into the data segment
inline constexpr std::uint16_t SPRITE_SEGMENT{ 0x361c };     // unit, order and effect sprites and their masks
inline constexpr std::uint16_t FRAME_SEGMENT{ 0x349b };      // map tiles, cell frames and dialog frames
inline constexpr std::uint16_t BACKGROUND_SEGMENT{ 0x3a64 }; // saved screen backgrounds (plane sprites, the weather)
inline constexpr std::uint16_t PATH_SEGMENT{ 0x4582 };       // the movement path-cost grid and its queue
} // namespace opeu::game::detail::memory

namespace opeu::game {
using detail::memory::BACKGROUND_SEGMENT;
using detail::memory::DATA_SEGMENT;
using detail::memory::FRAME_SEGMENT;
using detail::memory::PATH_SEGMENT;
using detail::memory::SPRITE_SEGMENT;
} // namespace opeu::game
