#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_deploy_unit(uint8_t* ds, BattleEvent event, uint16_t unit);

// The services of this module.
uint16_t svc_dplunit(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count);
