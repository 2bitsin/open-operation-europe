#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <cstdint>

namespace opeu::game::detail::btlaiact {
// The cost of the path flood's cheapest step from the cell (x, y); the cell stepped to goes to a reserve nobody
// reads.
auto BattlePathStep(Game& game, std::uint8_t x, std::uint8_t y) -> std::uint16_t;
} // namespace opeu::game::detail::btlaiact

namespace opeu::game {
using detail::btlaiact::BattlePathStep;
} // namespace opeu::game

uint16_t opeu_battle_ai_withdraw(Game& game, BattleEvent event, uint16_t element, uint16_t unit);
uint16_t opeu_battle_ai_try_fire(Game& game, BattleEvent event, uint16_t element, uint16_t target);
void     opeu_battle_ai_fire_or_attack(Game& game, BattleEvent event, uint16_t element, uint16_t unit);
uint16_t opeu_battle_ai_try_move(Game& game, BattleEvent event, uint16_t element, uint8_t x, uint8_t y);
void     opeu_battle_ai_advance(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_ai_engage_best(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_ai_act(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_ai_think(Game& game, BattleEvent event, uint16_t element);

// The services of the battle AI modules and of this module.
uint16_t svc_battle_ai(Game& game, uint32_t address, const uint16_t* args, unsigned count);
uint16_t svc_btlaiact(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
