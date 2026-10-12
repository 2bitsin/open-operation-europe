#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint8_t opeu_deploy_true(uint8_t* ds, BattleEvent event);
void    opeu_deploy_draw_rows(uint8_t* ds, BattleEvent event, uint8_t column,
                              uint8_t first, uint8_t row, uint8_t last,
                              uint8_t flags);

// The services of this module.
uint16_t svc_dplrows(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count);
