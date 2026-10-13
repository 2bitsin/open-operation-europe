// The computer side's planning: air strikes, airborne drops and the cells and types they use.
#pragma once

#include "common/events.hpp"
#include "core/aibudget.hpp"
#include "core/daylist.hpp"
#include "map/objeleg.hpp"
#include "types.hpp"
#include "units/release.hpp"

#include <cstdint>
#include <utility>

uint16_t opeu_ai_spare_cargo(Game& game);
uint16_t opeu_ai_002a(Game& game);
uint8_t  opeu_ai_first_allowed_type(Game& game);
uint16_t opeu_ai_exposed_enemy_objective(Game& game);
uint16_t opeu_ai_free_cell_near(Game& game, uint16_t objective, uint8_t* x, uint8_t* y);
uint16_t opeu_ai_03e2(Game& game);

namespace opeu::game::detail::ai_service {

// Lists the current side's units with none of the mask's flags into the list at `list`; returns the count.
inline auto AiFreeUnits(Game& game, std::uint16_t list, std::uint8_t mask) -> std::uint8_t {
  return opeu_release_side_units(game, list, mask);
}
// Lists the current side's elements with none of the mask's flags into the list at `list`; returns the count.
inline auto AiFreeElements(Game& game, std::uint16_t list, std::uint8_t mask) -> std::uint8_t {
  return opeu_release_side_elements(game, list, mask);
}
inline auto AiObjectiveLevel(Game& game, std::uint16_t objective) -> std::uint16_t {
  return opeu_objective_level(game, objective);
}
inline auto AiObjectiveEligible(Game& game, std::uint16_t objective) -> std::uint16_t {
  return opeu_objective_eligible(game, objective);
}
inline auto AiSupplyShare(Game& game, std::uint16_t* budget, std::uint16_t amount, std::uint8_t which) -> std::uint16_t {
  return opeu_ai_supply_share(game, budget, amount, which);
}
} // namespace opeu::game::detail::ai_service

namespace opeu::game {
using detail::ai_service::AiFreeElements;
using detail::ai_service::AiFreeUnits;
using detail::ai_service::AiObjectiveEligible;
using detail::ai_service::AiObjectiveLevel;
using detail::ai_service::AiSupplyShare;
} // namespace opeu::game
