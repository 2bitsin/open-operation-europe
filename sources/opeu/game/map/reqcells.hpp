#pragma once

#include "types.hpp"

uint16_t opeu_count_request_cells(uint8_t* ds);
uint16_t opeu_request_cells(uint8_t* ds, uint8_t side,
                            uint16_t (*predicate)(uint8_t*, uint8_t, uint8_t,
                                                  uint8_t));

// The services of this module.
uint16_t svc_reqcells(uint8_t* a0, uint8_t a1, uint8_t a2, uint8_t a3);
