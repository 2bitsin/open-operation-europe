// The battle routines several battle modules call, with their results narrowed to the bytes they are.
#pragma once

#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcalc.hpp"

#include <cstdint>

namespace opeu::game::detail::battle_calls {

// The grid direction 0..7 from the battle's map cell to the unit's cell, or BATTLE_NOT_ADJACENT.
[[nodiscard]] inline auto UnitDirection(Game& game, std::uint16_t unit) -> std::uint8_t {
  return static_cast<std::uint8_t>(opeu_battle_unit_direction(game, svc_btlcalc, unit));
}

// The element's crew skill of one kind (battle_skill_kinds).
[[nodiscard]] inline auto ElementSkill(Game& game, std::uint16_t element, std::uint8_t kind) -> std::uint8_t {
  return static_cast<std::uint8_t>(opeu_battle_skill(game, svc_btlcalc, element, static_cast<uint8_t>(kind)));
}

// The element's battle strength.
[[nodiscard]] inline auto ElementStrength(Game& game, std::uint16_t element) -> std::uint8_t {
  return static_cast<std::uint8_t>(opeu_battle_element_strength(game, svc_btlcalc, element));
}

} // namespace opeu::game::detail::battle_calls

namespace opeu::game {
using detail::battle_calls::ElementSkill;
using detail::battle_calls::ElementStrength;
using detail::battle_calls::UnitDirection;
} // namespace opeu::game
