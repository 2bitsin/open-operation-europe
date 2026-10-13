// The fixture's battle routines after the battle sites became direct calls.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void          opeu_step(Game& game, BattleEvent event, std::uint16_t element);
void          opeu_mark(Game& game, BattleEvent, std::uint16_t cell);
void          opeu_show(Game& game, BattleEvent event);
std::uint16_t opeu_battle_turn(Game& game, BattleEvent event, std::uint16_t element);

// The services of this module.
std::uint16_t svc_battle(Game& game, std::uint32_t address, std::uint16_t const* args, unsigned count);
