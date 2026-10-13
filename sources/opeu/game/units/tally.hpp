// Per-type tallies of unit elements and objective stock for the allocation screens.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_tally_mark(Game& game);
uint8_t opeu_tally_elements(Game& game, uint16_t unit, uint8_t count);
uint8_t opeu_tally_stock(Game& game, uint16_t source_offset, uint8_t count, uint8_t kind);
void    opeu_tally_average(Game& game, uint8_t count);
uint8_t opeu_tally_unit_stock(Game& game, HostEvent event, uint16_t unit, uint16_t source_offset);
uint8_t opeu_tally_units(Game& game, HostEvent event, uint16_t unit, uint16_t other);

// The services the module calls out to.
uint16_t svc_tally(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
