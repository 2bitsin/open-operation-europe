// Distributing allocated equipment over units and objectives.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_alloc_distribute(uint8_t* ds, HostWordsEvent event, uint8_t first, uint8_t second, uint8_t third,
                           uint8_t count);
void opeu_alloc_choose_hq(uint8_t* ds, HostWordsEvent event, uint16_t text, uint16_t name, uint16_t callback_segment);
void opeu_alloc_finish(uint8_t* ds, HostWordsEvent event);

// The services the module calls out to.
uint16_t svc_allocdst(uint8_t* ds, uint16_t id, uint16_t* args, uint16_t count);
