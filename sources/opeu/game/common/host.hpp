// The machine-side layout the UI primitives and the dialogs read: where far memory is, the segment numbers of the data
// segment and the frame tiles, and the data-segment area of the dialogs' buffers. Members of Game (common/game.hpp);
// B7 replaces the flat memory and the segment numbers with typed far buffers.
#pragma once

#include "port.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace opeu::game::detail::host {

inline constexpr std::uint16_t FRAME_TILES_SEGMENT{ 0x3873 }; // the far segment of the frame tiles (data/fg3873)
inline constexpr std::size_t   ARENA_SLACK{ 0x10000 };        // the bytes past the arena a segment:offset near its top reaches

// The dialogs' load segment and the data-segment area of their buffers: the bottom of the 5120-byte stack, which the
// stack never reaches (at most 132 bytes; one buffer owner is active at a time).
struct DialogHost {
  std::uint16_t load{ 0 };
  std::uint16_t scratch{ PORT_DS_END };
};

// The UI primitives' far memory (linear = segment * 16 + offset) and the segment numbers they address it by.
struct UiHost {
  std::span<std::uint8_t> memory;
  std::uint16_t           ds_segment{ PORT_DS_SEGMENT };
  std::uint16_t           tiles_segment{ FRAME_TILES_SEGMENT };
};

// The glue's far-data arena as the UI primitives' far memory, its slack included (glue/core/memory.cpp); no far memory
// before the arena is prepared.
inline auto ArenaUi() -> UiHost {
  if (port_arena == nullptr) return UiHost{};
  return UiHost{
    .memory = std::span{ port_arena, PORT_ARENA_SIZE + ARENA_SLACK }
  };
}

} // namespace opeu::game::detail::host

namespace opeu::game {
using detail::host::ArenaUi;
using detail::host::DialogHost;
using detail::host::FRAME_TILES_SEGMENT;
using detail::host::UiHost;
} // namespace opeu::game
