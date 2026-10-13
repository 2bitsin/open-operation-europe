#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_objective_loss_line(Game& game, uint16_t objective, uint8_t kind, uint16_t amount);
void opeu_objective_damage(Game& game, uint16_t objective, uint16_t chance, uint16_t base, uint16_t supply, uint16_t endurance, uint16_t spread, uint16_t heavy);
