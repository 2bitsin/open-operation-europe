// The saved-game file: twenty slot records, then for each slot a copy of the block of the data segment a saved game
// keeps (scenario_first through save_last), with its record pointers stored as indices.
#pragma once

#include "common/data-segment.hpp"

#include <cstddef>
#include <cstdint>

namespace opeu::game::detail::saved_game {

// The block of the data segment a saved game keeps.
inline constexpr std::uint16_t SAVED_FIRST{ offsetof(DataSegment,
                                                     scenario_first) };
inline constexpr std::uint16_t SAVED_LAST{ offsetof(DataSegment, save_last) };
inline constexpr std::uint16_t SAVED_BYTES{ SAVED_LAST - SAVED_FIRST + 1 };

// The file offset of the first saved block, after the slot records.
inline constexpr std::uint16_t SAVED_BLOCKS_AT{ SAVE_SLOT_COUNT *
                                                sizeof(SaveSlot) };

// SaveSlot::scenario of a slot that holds no game.
inline constexpr std::uint8_t SLOT_EMPTY{ 6 };

// What an order's target word points to, by the order's kind: a saved game stores the target as the index of that
// record.
enum class OrderTarget : std::uint8_t {
  NONE,      // the word is not a record pointer
  OBJECTIVE, // an objective
  UNIT,      // a unit
};

// The target of a side's order.
[[nodiscard]] constexpr auto SideOrderTarget(Order const& order) noexcept
    -> OrderTarget {
  switch (order.kind) {
    case 0:
    case 2:
    case 3:
    case 6:  return OrderTarget::OBJECTIVE;
    case 1:
    case 4:
    case 5:
    case 7:  return OrderTarget::UNIT;
    default: return OrderTarget::NONE;
  }
}

// The target of a unit's order: some kinds aim at another unit, the others at a map cell.
[[nodiscard]] constexpr auto UnitOrderTarget(Order const& order) noexcept
    -> OrderTarget {
  switch (order.kind) {
    case 2:
    case 9:
    case 11:
    case 12: return OrderTarget::UNIT;
    default: return OrderTarget::NONE;
  }
}

// The byte sum of the saved block, as the slot's checksum keeps it.
[[nodiscard]] inline auto SavedChecksum(DataSegment const& data) noexcept
    -> std::uint16_t {
  auto const* const bytes{ reinterpret_cast<std::uint8_t const*>(&data) };
  std::uint16_t     sum{ 0 };
  for (auto at{ SAVED_FIRST }; at <= SAVED_LAST; at++)
    sum = static_cast<std::uint16_t>(sum + bytes[at]);
  return sum;
}

} // namespace opeu::game::detail::saved_game

namespace opeu::game {
using detail::saved_game::OrderTarget;
using detail::saved_game::SAVED_BLOCKS_AT;
using detail::saved_game::SAVED_BYTES;
using detail::saved_game::SAVED_FIRST;
using detail::saved_game::SAVED_LAST;
using detail::saved_game::SavedChecksum;
using detail::saved_game::SideOrderTarget;
using detail::saved_game::SLOT_EMPTY;
using detail::saved_game::UnitOrderTarget;
} // namespace opeu::game
