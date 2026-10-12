#pragma once

#include "types.hpp"

void opeu_update_influence_map(uint8_t* ds);
void opeu_turn_hourly_c(uint8_t* ds,
                        uint16_t (*event)(uint8_t*, uint16_t, uint16_t,
                                          uint16_t, uint16_t, uint16_t));

// The services of this module.
uint16_t svc_influenc(uint8_t* ds, uint16_t id, uint16_t a, uint16_t b,
                      uint16_t c, uint16_t d);
