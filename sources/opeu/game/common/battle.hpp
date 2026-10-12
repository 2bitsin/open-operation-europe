// The battle field: each engaged element's position record and the 14 x 14 battle cells.
#pragma once

#include "common/data-segment.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace opeu::game::detail::battle {

#pragma pack(push, 1)
// An engaged element's position record (battle_positions, ten bytes each; Element::position points to it).
struct BattlePosition {
  std::uint8_t x; // BATTLE_OFF_FIELD set once the element has left the field
  std::uint8_t y; // BATTLE_OFF_FIELD set once the element has left the field
  std::uint8_t facing; // BattleFacing
  std::uint8_t unknown_3;
  std::uint8_t strength; // low nibble and high nibble (map/btlunit)
  union {
    std::uint16_t               target; // the element the order is against
    std::array<std::uint8_t, 2> goal;   // or the cell (x, y) it leads to
  };
  std::uint8_t state; // BattleState, BATTLE_ENGAGED on top
  std::uint8_t order; // the order level (map/btlunit)
  std::uint8_t mode;  // 0..3
};
#pragma pack(pop)
static_assert(sizeof(BattlePosition) == 10);
static_assert(offsetof(BattlePosition, target) == 5);
static_assert(offsetof(BattlePosition, state) == 7);
static_assert(offsetof(BattlePosition, mode) == 9);

// BattlePosition::state & BATTLE_STATE_MASK: what the element does each round (map/btladv dispatches on it).
enum class BattleState : std::uint8_t {
  ADVANCE  = 0,
  CONTINUE = 1, // carries on with its order
  SEEK     = 2, // looks for a target
  WITHDRAW = 3,
  HOLD     = 4,
  RALLY    = 5,
};
inline constexpr std::uint8_t BATTLE_STATE_MASK{ 0x7f };
inline constexpr std::uint8_t BATTLE_ENGAGED{
  0x80
}; // on BattlePosition::state
inline constexpr std::uint8_t BATTLE_OFF_FIELD{
  0x80
}; // on BattlePosition::x and y

// BattlePosition::facing, and the direction from one cell toward another.
enum class BattleFacing : std::uint8_t {
  UP    = 0,
  DOWN  = 1,
  LEFT  = 2,
  RIGHT = 3,
};

// Bits of a battle cell: the low two bits are the side standing there plus one; the high nibble is the strategic
// direction plus one an element leaving the field from there takes.
enum class BattleCell : std::uint8_t {
  OCCUPANT_MASK = 0x03,
  EXIT_SHIFT    = 4,
};

// The position record of an element.
[[nodiscard]] inline auto Position(DataSegment&  data,
                                   std::uint16_t element) noexcept
    -> BattlePosition& {
  return Record<BattlePosition>(data, Record<Element>(data, element).position);
}

// The battle cell (x, y).
[[nodiscard]] inline auto BattleCellAt(DataSegment& data, std::uint8_t x,
                                       std::uint8_t y) noexcept
    -> std::uint8_t& {
  return Record<std::uint8_t>(
      data, static_cast<std::uint16_t>(Near(data, data.battle_cells) + x +
                                       y * BATTLE_WIDTH));
}
} // namespace opeu::game::detail::battle

namespace opeu::game {
using detail::battle::BATTLE_ENGAGED;
using detail::battle::BATTLE_OFF_FIELD;
using detail::battle::BATTLE_STATE_MASK;
using detail::battle::BattleCell;
using detail::battle::BattleCellAt;
using detail::battle::BattleFacing;
using detail::battle::BattlePosition;
using detail::battle::BattleState;
using detail::battle::Position;
} // namespace opeu::game
