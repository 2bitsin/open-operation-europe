// The saved-game file: twenty slot records, then for each slot a copy of the block of the data segment a saved game
// keeps (scenario_first through save_last), with its record pointers stored as indices.
#pragma once

#include "common/data-segment.hpp"

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>

namespace opeu::game::detail::saved_game {

// The block of the data segment a saved game keeps.
inline constexpr std::uint16_t SAVED_FIRST{ offsetof(DataSegment, scenario_first) };
inline constexpr std::uint16_t SAVED_LAST{ offsetof(DataSegment, save_last) };
inline constexpr std::uint16_t SAVED_BYTES{ SAVED_LAST - SAVED_FIRST + 1 };

// The file offset of the first saved block, after the slot records.
inline constexpr std::uint16_t SAVED_BLOCKS_AT{ SAVE_SLOT_COUNT * sizeof(SaveSlot) };

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
[[nodiscard]] constexpr auto SideOrderTarget(Order const& order) noexcept -> OrderTarget {
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
[[nodiscard]] constexpr auto UnitOrderTarget(Order const& order) noexcept -> OrderTarget {
  switch (order.kind) {
  case 2:
  case 9:
  case 11:
  case 12: return OrderTarget::UNIT;
  default: return OrderTarget::NONE;
  }
}

// A stored record index that stands for a null pointer.
inline constexpr std::uint16_t STORED_NULL{ 0xffff };

// The record index a saved game stores for a near pointer to the table of T at `base` (STORED_NULL for null). The
// pointer difference is divided as a signed word.
template <typename T>
[[nodiscard]] constexpr auto StoredIndex(std::uint16_t near, std::uint16_t base) noexcept -> std::uint16_t {
  if (near == 0) return STORED_NULL;
  return static_cast<std::uint16_t>(static_cast<std::int16_t>(static_cast<std::uint16_t>(near - base)) / static_cast<std::int16_t>(sizeof(T)));
}

// The near pointer of the stored record `index` of the table of T at `base` (null for STORED_NULL), wrapping in 16-bit
// arithmetic.
template <typename T>
[[nodiscard]] constexpr auto StoredRecord(std::uint16_t index, std::uint16_t base) noexcept -> std::uint16_t {
  if (index == STORED_NULL) return 0;
  return static_cast<std::uint16_t>(base + index * sizeof(T));
}

// The stored index of an order's target, and the target's near pointer back, by what the order's kind points to.
[[nodiscard]] constexpr auto StoredTarget(std::uint16_t near, OrderTarget target) noexcept -> std::uint16_t {
  return target == OrderTarget::OBJECTIVE ? StoredIndex<Objective>(near, OBJECTIVES_AT) : StoredIndex<Unit>(near, UNITS_AT);
}
[[nodiscard]] constexpr auto TargetRecord(std::uint16_t index, OrderTarget target) noexcept -> std::uint16_t {
  return target == OrderTarget::OBJECTIVE ? StoredRecord<Objective>(index, OBJECTIVES_AT) : StoredRecord<Unit>(index, UNITS_AT);
}

// The byte sum of the saved block, as the slot's checksum keeps it.
[[nodiscard]] inline auto SavedChecksum(DataSegment const& data) noexcept -> std::uint16_t {
  auto const saved{ std::as_bytes(std::span{ &data, 1 }).subspan(SAVED_FIRST, SAVED_BYTES) };
  return std::accumulate(saved.begin(), saved.end(), std::uint16_t{ 0 }, [](std::uint16_t sum, std::byte byte) { return static_cast<std::uint16_t>(sum + std::to_integer<std::uint8_t>(byte)); });
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
using detail::saved_game::STORED_NULL;
using detail::saved_game::StoredIndex;
using detail::saved_game::StoredRecord;
using detail::saved_game::StoredTarget;
using detail::saved_game::TargetRecord;
using detail::saved_game::UnitOrderTarget;
} // namespace opeu::game
