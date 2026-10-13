// The battle routines several battle modules reach the same way through their BattleEvent, with typed results.
#pragma once

#include "common/battle-routine.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"

#include <cstdint>

namespace opeu::game::detail::battle_calls {

// The grid direction 0..7 from the battle's map cell to the unit's cell, or BATTLE_NOT_ADJACENT.
[[nodiscard]] inline auto UnitDirection(std::uint8_t* ds, BattleEvent event, std::uint16_t unit) -> std::uint8_t {
  return static_cast<std::uint8_t>(Call(ds, event, BattleRoutine::BATTLE_UNIT_DIRECTION, unit));
}

// The element's crew skill of one kind (battle_skill_kinds).
[[nodiscard]] inline auto ElementSkill(std::uint8_t* ds, BattleEvent event, std::uint16_t element, std::uint8_t kind) -> std::uint8_t {
  return static_cast<std::uint8_t>(Call(ds, event, BattleRoutine::BATTLE_SKILL, kind, element));
}

// The element's battle strength.
[[nodiscard]] inline auto ElementStrength(std::uint8_t* ds, BattleEvent event, std::uint16_t element) -> std::uint8_t {
  return static_cast<std::uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_STRENGTH, element));
}

} // namespace opeu::game::detail::battle_calls

namespace opeu::game {
using detail::battle_calls::ElementSkill;
using detail::battle_calls::ElementStrength;
using detail::battle_calls::UnitDirection;
} // namespace opeu::game
