// The battle field: the position record each element in a battle points to, the 14 x 14 cell grid and its terrain.
#pragma once

#include "common/data-segment.hpp"

#include <cstdint>

namespace opeu::game::detail::battle_field {

// An element's battle record (DS:d0f8, 60 x 10 bytes); Element::position points to it while a battle runs.
#pragma pack(push, 1)
struct BattlePosition {
  std::uint8_t x; // column; BATTLE_UNPLACED before deployment, BATTLE_LEFT set once the element left the field
  union {
    std::uint8_t y;      // row; BATTLE_UNPLACED before deployment
    std::uint8_t rounds; // in a battle fought out of sight: the rounds between the element's shots (map/btlend)
  };
  std::uint8_t facing; // one of the six hex directions
  std::uint8_t unknown_3;
  std::uint8_t strength; // two nibbles (map/btlunit)
  union {
    struct {
      std::uint8_t goal_x; // the cell its order heads for
      std::uint8_t goal_y;
    };
    std::uint16_t target; // or the element its order is against
  };
  std::uint8_t state; // BattleState
  std::uint8_t order; // the battle order
  std::uint8_t mode;  // 0..3
};
#pragma pack(pop)
static_assert(sizeof(BattlePosition) == 10);

// BattlePosition::x and y of an element not on the field.
inline constexpr std::uint8_t BATTLE_UNPLACED{ 0xff };
// The bit of BattlePosition::x of an element that left the field.
inline constexpr std::uint8_t BATTLE_LEFT{ 0x80 };

// BattlePosition::state: the element's battle order in the low seven bits (the player's orders, map/btlcmds), and
// the engaged bit.
enum class BattleState : std::uint8_t {
  MOVE       = 0,
  ATTACK     = 1,
  BLITZ      = 2,
  WITHDRAW   = 3,
  DEFEND     = 4,
  REST       = 5,
  STATE_MASK = 0x7f,
  ENGAGED    = 0x80,
};

// The direction from one battle cell toward another (opeu_battle_cell_facing, opeu_battle_element_facing).
enum class CellFacing : std::uint8_t {
  UP    = 0,
  DOWN  = 1,
  LEFT  = 2,
  RIGHT = 3,
};

// What opeu_battle_unit_direction returns for a unit not next to the battle's map cell (otherwise the grid direction
// 0..7 from the battle cell to the unit's cell).
inline constexpr std::uint8_t BATTLE_NOT_ADJACENT{ 9 };

// Bits of a battle cell (battle_cells): the low two bits hold the occupying side + 1.
enum class BattleCell : std::uint8_t {
  OCCUPANT_MASK = 0x03,
  EXIT_SHIFT    = 4, // the high nibble: the strategic direction + 1 an element leaving the field from there takes
};

// battle_terrain: the low six bits hold the terrain code, the top two bits are kept from the hex map.
inline constexpr std::uint8_t BATTLE_TERRAIN_MASK{ 0x3f };
// The battle terrain code of the objective's cells; its garrison is shared among them (map/btlmap).
inline constexpr std::uint8_t BATTLE_CITY{ 1 };

// ElementType::gun_range above this fires from afar.
inline constexpr std::uint8_t RANGED_MODE{ 1 };

// The battle record of an element.
[[nodiscard]] inline auto PositionOf(DataSegment& data, std::uint16_t element) noexcept -> BattlePosition& {
  return Record<BattlePosition>(data, Record<Element>(data, element).position);
}

// The battle cell byte at (x, y), addressed as the original does (a column past the edge reaches the next row).
[[nodiscard]] inline auto BattleCellAt(DataSegment& data, unsigned x, unsigned y) noexcept -> std::uint8_t& {
  return Record<std::uint8_t>(data, static_cast<std::uint16_t>(Near(data, data.battle_cells) + y * BATTLE_WIDTH + x));
}

// The battle terrain byte at (x, y).
[[nodiscard]] inline auto BattleTerrainAt(DataSegment& data, unsigned x, unsigned y) noexcept -> std::uint8_t& {
  return Record<std::uint8_t>(data, static_cast<std::uint16_t>(Near(data, data.battle_terrain) + y * BATTLE_WIDTH + x));
}

// The side an element fights for in a battle.
[[nodiscard]] inline auto BattleSide(Element const& element) noexcept -> std::uint8_t {
  return element.flags & Bits(ElementFlag::SIDE_1) ? 1 : 0;
}

// A cell of the battle's objective and the garrison share placed on it (battle_terrain_extras, 10 records; x is
// BATTLE_UNPLACED in the unused ones, map/btlmap).
struct BattleExtra {
  std::uint8_t x;
  std::uint8_t y;
  std::uint8_t count;
};
inline constexpr unsigned BATTLE_EXTRA_COUNT{ 10 };
[[nodiscard]] inline auto BattleExtraAt(DataSegment& data, unsigned i) noexcept -> BattleExtra& {
  return Record<BattleExtra>(data, static_cast<std::uint16_t>(Near(data, data.battle_terrain_extras) + i * sizeof(BattleExtra)));
}

// The battle's unit lists: battle_lists[side * 9 + i], 0 after the last.
inline constexpr unsigned BATTLE_LIST_LENGTH{ 9 };
[[nodiscard]] inline auto BattleList(DataSegment& data, unsigned side, unsigned i) noexcept -> std::uint16_t& {
  return data.battle_lists[side * BATTLE_LIST_LENGTH + i];
}

} // namespace opeu::game::detail::battle_field

namespace opeu::game {
using detail::battle_field::BATTLE_CITY;
using detail::battle_field::BATTLE_LEFT;
using detail::battle_field::BATTLE_LIST_LENGTH;
using detail::battle_field::BATTLE_NOT_ADJACENT;
using detail::battle_field::BATTLE_TERRAIN_MASK;
using detail::battle_field::BATTLE_UNPLACED;
using detail::battle_field::BattleCell;
using detail::battle_field::BattleCellAt;
using detail::battle_field::BATTLE_EXTRA_COUNT;
using detail::battle_field::BattleExtra;
using detail::battle_field::BattleExtraAt;
using detail::battle_field::BattleList;
using detail::battle_field::BattlePosition;
using detail::battle_field::BattleSide;
using detail::battle_field::BattleState;
using detail::battle_field::CellFacing;
using detail::battle_field::BattleTerrainAt;
using detail::battle_field::PositionOf;
using detail::battle_field::RANGED_MODE;
} // namespace opeu::game
