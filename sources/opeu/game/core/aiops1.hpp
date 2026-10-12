// The computer side's planning: air strikes, airborne drops and the cells and types they use.
#pragma once

#include "types.hpp"

#include <cstdint>
#include <utility>

// The routines the computer side's planning reaches through this pointer (svc_ai_unrecovered); bx is the BX
// argument, a DS offset (for the supply share the budget word, in and out).
extern uint16_t (*opeu_ai_unrecovered)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t*);
uint16_t opeu_ai_spare_cargo(uint8_t* ds);
uint16_t opeu_ai_002a(uint8_t* ds);
uint8_t  opeu_ai_first_allowed_type(uint8_t* ds);
uint16_t opeu_ai_exposed_enemy_objective(uint8_t* ds);
uint16_t opeu_ai_free_cell_near(uint8_t* ds, uint16_t objective, uint8_t* x, uint8_t* y);
uint16_t opeu_ai_03e2(uint8_t* ds);

// Runs the routines opeu_ai_unrecovered reaches.
uint16_t svc_ai_unrecovered(uint8_t* ds, uint16_t id, uint16_t ax, uint16_t dx, uint16_t* bx);

namespace opeu::game::detail::ai_service {

// The routines behind opeu_ai_unrecovered.
enum class AiService : std::uint16_t {
  RELEASE_UNITS      = 0, // list the side's free units into the list at bx; ax: the flag mask -> the count
  RELEASE_ELEMENTS   = 1, // list the side's free elements into the list at bx; ax: the flag mask -> the count
  DUE_RECORDS        = 2, // list the dated records due (bx: the list, ax, dx) -> the count
  OBJECTIVE_LEVEL    = 3, // the level of the objective bx
  OBJECTIVE_ELIGIBLE = 4, // whether the objective bx may be planned for
  SUPPLY_SHARE       = 5, // opeu_ai_supply_share(bx: &budget, ax: amount, dx: which)
};

inline auto AiCall(std::uint8_t* ds, AiService service, std::uint16_t ax, std::uint16_t dx, std::uint16_t* bx) -> std::uint16_t {
  return opeu_ai_unrecovered(ds, std::to_underlying(service), ax, dx, bx);
}

// Lists the current side's units with none of the mask's flags into the list at `list`; returns the count.
inline auto AiFreeUnits(std::uint8_t* ds, std::uint16_t list, std::uint8_t mask) -> std::uint8_t {
  return static_cast<std::uint8_t>(AiCall(ds, AiService::RELEASE_UNITS, mask, 0, &list));
}
// Lists the current side's elements with none of the mask's flags into the list at `list`; returns the count.
inline auto AiFreeElements(std::uint8_t* ds, std::uint16_t list, std::uint8_t mask) -> std::uint8_t {
  return static_cast<std::uint8_t>(AiCall(ds, AiService::RELEASE_ELEMENTS, mask, 0, &list));
}
inline auto AiObjectiveLevel(std::uint8_t* ds, std::uint16_t objective) -> std::uint16_t {
  return AiCall(ds, AiService::OBJECTIVE_LEVEL, 0, 0, &objective);
}
inline auto AiObjectiveEligible(std::uint8_t* ds, std::uint16_t objective) -> std::uint16_t {
  return AiCall(ds, AiService::OBJECTIVE_ELIGIBLE, 0, 0, &objective);
}
inline auto AiSupplyShare(std::uint8_t* ds, std::uint16_t* budget, std::uint16_t amount, std::uint8_t which) -> std::uint16_t {
  return AiCall(ds, AiService::SUPPLY_SHARE, amount, which, budget);
}
} // namespace opeu::game::detail::ai_service

namespace opeu::game {
using detail::ai_service::AiCall;
using detail::ai_service::AiFreeElements;
using detail::ai_service::AiFreeUnits;
using detail::ai_service::AiObjectiveEligible;
using detail::ai_service::AiObjectiveLevel;
using detail::ai_service::AiService;
using detail::ai_service::AiSupplyShare;
} // namespace opeu::game
