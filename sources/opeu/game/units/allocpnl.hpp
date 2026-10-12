// The allocation screen's type panel and the allocation arithmetic.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void     opeu_alloc_clear_panel(uint8_t* ds, HostWordsEvent event);
void     opeu_alloc_type_panel(uint8_t* ds, HostWordsEvent event, uint8_t slot,
                               uint16_t type);
uint16_t opeu_alloc_groups_nonempty(uint8_t* ds, HostWordsEvent event,
                                    uint8_t first, uint8_t second);
void     opeu_alloc_element_values(uint8_t* ds, HostWordsEvent event,
                                   uint16_t element, uint8_t count);
uint16_t opeu_alloc_scaled(uint8_t* ds, HostWordsEvent event, int32_t value,
                           uint16_t amount, int32_t total);

uint16_t svc_allocpnl(uint8_t* ds, uint16_t id, uint16_t* args, uint16_t count);
