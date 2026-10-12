// The supply transfer's step and its driver.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void     opeu_transfer_step(uint8_t* ds, HostWordsEvent event);
uint16_t opeu_transfer_run(uint8_t* ds, HostWordsEvent event,
                           uint16_t interactive);

uint16_t svc_xferstep(uint8_t* ds, uint16_t id, uint16_t* args, uint16_t count);
