#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_deploy_true(Game& game, BattleEvent event);
void    opeu_deploy_draw_rows(Game& game, BattleEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);

// The services of this module.
uint16_t svc_dplrows(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
