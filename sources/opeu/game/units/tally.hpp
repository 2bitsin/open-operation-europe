// Per-type tallies of unit elements and objective stock for the allocation screens.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint8_t opeu_tally_mark(uint8_t* ds);
uint8_t opeu_tally_elements(uint8_t* ds, uint16_t unit, uint8_t count);
uint8_t opeu_tally_stock(uint8_t* ds, uint16_t source_offset, uint8_t count,
                         uint8_t kind);
void    opeu_tally_average(uint8_t* ds, uint8_t count);
uint8_t opeu_tally_unit_stock(uint8_t* ds, HostEvent event, uint16_t unit,
                              uint16_t source_offset);
uint8_t opeu_tally_units(uint8_t* ds, HostEvent event, uint16_t unit,
                         uint16_t other);

// The services the module calls out to.
uint16_t svc_tally(uint8_t* ds, uint16_t id, const uint16_t* args,
                   uint16_t count);
