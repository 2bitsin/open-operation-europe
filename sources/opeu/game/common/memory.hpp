// Service locals and far segments on MAIN's 16-bit data segment, shared by the game files.
#pragma once

#include "common/game.hpp"

#include "port.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

namespace opeu::game::detail::memory {

// Paragraphs of the far data the game addresses as segment:offset pairs (unrelocated, as the services expect them).
inline constexpr std::uint16_t DATA_SEGMENT{ 0x4712 };       // DS (= SS): a far pointer into the data segment
inline constexpr std::uint16_t SPRITE_SEGMENT{ 0x361c };     // unit, order and effect sprites and their masks
inline constexpr std::uint16_t FRAME_SEGMENT{ 0x349b };      // map tiles, cell frames and dialog frames
inline constexpr std::uint16_t BACKGROUND_SEGMENT{ 0x3a64 }; // saved screen backgrounds (plane sprites, the weather)
inline constexpr std::uint16_t PATH_SEGMENT{ 0x4582 };       // the movement path-cost grid and its queue

// The DS bytes a routine reserves on the DS stack for its locals whose near address it hands to a service: the
// service reads and writes them in the data segment. Only those locals live here (#13 gives the services typed
// parameters and removes it); every other local is a C++ local.
class ServiceLocals {
public:
  explicit ServiceLocals(std::uint16_t bytes) : _bytes{ bytes }, _base{ static_cast<std::uint16_t>(port_stack_enter(bytes) - bytes) } { }
  ServiceLocals(ServiceLocals const&)                    = delete;
  auto operator=(ServiceLocals const&) -> ServiceLocals& = delete;
  ~ServiceLocals() {
    port_stack_leave(_bytes);
  }

  // Zeroes the reserve: for texts a service prints past their terminator.
  auto Clear(Game& game) const -> void {
    std::ranges::fill(game.State().Bytes(_base, _bytes), std::uint8_t{ 0 });
  }

  // The near address of the local `offset` bytes into the reserve.
  [[nodiscard]] auto At(std::uint16_t offset) const noexcept -> std::uint16_t {
    return static_cast<std::uint16_t>(_base + offset);
  }

private:
  std::uint16_t _bytes;
  std::uint16_t _base;
};
} // namespace opeu::game::detail::memory

namespace opeu::game {
using detail::memory::BACKGROUND_SEGMENT;
using detail::memory::DATA_SEGMENT;
using detail::memory::FRAME_SEGMENT;
using detail::memory::PATH_SEGMENT;
using detail::memory::ServiceLocals;
using detail::memory::SPRITE_SEGMENT;
} // namespace opeu::game
