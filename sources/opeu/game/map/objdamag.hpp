#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_objective_loss_line(Game& game, HostEvent event, uint16_t objective, uint8_t kind, uint16_t amount);
void opeu_objective_damage(Game& game, HostEvent event, uint16_t objective, uint16_t chance, uint16_t base, uint16_t supply, uint16_t endurance, uint16_t spread, uint16_t heavy);

// The services of this module.
uint16_t svc_objdamag(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
