// The paratroop regiment: forming it, judging landing cells, and landing it.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_para_unit_create(uint8_t* ds, HostWordsEvent event, uint8_t side,
                               uint8_t x, uint8_t y);
uint16_t opeu_para_cell_ok(uint8_t* ds, HostWordsEvent event, uint8_t x,
                           uint8_t y);
uint16_t opeu_para_scatter_check(uint8_t* ds, HostWordsEvent event);
uint16_t opeu_para_land(uint8_t* ds, HostWordsEvent event, uint16_t unit,
                        uint16_t losses);

// The services the paratroop routines are called with.
uint16_t svc_paraunit(uint8_t* ds, uint16_t id, uint16_t* args, uint16_t count);
