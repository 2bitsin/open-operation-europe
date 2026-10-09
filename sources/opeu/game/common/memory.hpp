// Word access, frame locals, far segments and service calls on MAIN's 16-bit data segment, shared by the game files.
#pragma once

#include <cstdint>

namespace opeu::game::detail::memory {

// The word at a DS offset: little-endian, the offset of its high byte wraps at 16 bits.
[[nodiscard]] inline auto Word(std::uint8_t const* ds,
                               std::uint16_t at) noexcept -> std::uint16_t {
  return static_cast<std::uint16_t>(
      ds[at] | ds[static_cast<std::uint16_t>(at + 1)] << 8);
}

// Stores a word at a DS offset; returns the value stored.
inline auto SetWord(std::uint8_t* ds, std::uint16_t at,
                    std::uint16_t value) noexcept -> std::uint16_t {
  ds[at] = static_cast<std::uint8_t>(value);
  ds[static_cast<std::uint16_t>(at + 1)] =
      static_cast<std::uint8_t>(value >> 8);
  return value;
}

// The DS offset of a routine's local that lives `offset` bytes below its frame pointer on the DS stack. Locals stay
// there only where their near address is handed to another routine or service.
[[nodiscard]] constexpr auto Local(std::uint16_t frame,
                                   std::uint16_t offset) noexcept
    -> std::uint16_t {
  return static_cast<std::uint16_t>(frame - offset);
}

// Paragraphs of the far data the game addresses as segment:offset pairs (unrelocated, as the services expect them).
inline constexpr std::uint16_t DATA_SEGMENT{
  0x4712
}; // DS (= SS): a far pointer into the data segment
inline constexpr std::uint16_t SPRITE_SEGMENT{
  0x361c
}; // unit, order and effect sprites and their masks
inline constexpr std::uint16_t FRAME_SEGMENT{
  0x349b
}; // map tiles, cell frames and dialog frames

// Calls a service with its argument words (a count of 0 passes no array). For services that write results back into
// the argument array, build the array at the call site instead.
template <typename Event, typename Service, typename... Words>
inline auto Call(std::uint8_t* ds, Event event, Service service,
                 Words... words) {
  if constexpr (sizeof...(Words) == 0) {
    return event(ds, service, nullptr, 0);
  } else {
    std::uint16_t args[]{ static_cast<std::uint16_t>(words)... };
    return event(ds, service, args,
                 static_cast<std::uint16_t>(sizeof...(Words)));
  }
}
} // namespace opeu::game::detail::memory

namespace opeu::game {
using detail::memory::Call;
using detail::memory::DATA_SEGMENT;
using detail::memory::FRAME_SEGMENT;
using detail::memory::Local;
using detail::memory::SetWord;
using detail::memory::SPRITE_SEGMENT;
using detail::memory::Word;
} // namespace opeu::game
