// The equipment allocation screen.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_alloc_amounts_show(uint8_t* ds, HostWordsEvent event, uint8_t count);
void opeu_alloc_take(uint8_t* ds, HostWordsEvent event, uint16_t entry, uint8_t slot, uint16_t amount, uint8_t count);
void opeu_alloc_screen(uint8_t* ds, HostWordsEvent event, uint8_t first, uint8_t second, uint8_t third, uint8_t count,
                       uint16_t buffer);

// The services the module calls out to.
uint16_t svc_allocscr(uint8_t* ds, uint16_t id, uint16_t* args, uint16_t count);
