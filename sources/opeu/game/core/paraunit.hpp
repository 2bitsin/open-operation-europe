// The paratroop regiment: forming it, judging landing cells, and landing it.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_para_unit_create(Game& game, HostWordsEvent event, uint8_t side, uint8_t x, uint8_t y);
uint16_t opeu_para_cell_ok(Game& game, HostWordsEvent event, uint8_t x, uint8_t y);
uint16_t opeu_para_scatter_check(Game& game, HostWordsEvent event);
uint16_t opeu_para_land(Game& game, HostWordsEvent event, uint16_t unit, uint16_t losses);

// The services the paratroop routines are called with.
uint16_t svc_paraunit(Game& game, uint16_t id, uint16_t* args, uint16_t count);
