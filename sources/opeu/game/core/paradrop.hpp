// The paratroop drop: counting it down, flying it, and calling it off.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_para_clear(uint8_t* ds);
uint16_t opeu_para_ready(uint8_t* ds, HostEvent event);
uint16_t opeu_para_drop_due(uint8_t* ds, HostEvent event);
void     opeu_para_drop_present(uint8_t* ds, HostEvent event);
void     opeu_para_drop_event(uint8_t* ds, HostEvent event, uint16_t segment);

// The services the drop routines are called with.
uint16_t svc_paradrop(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
