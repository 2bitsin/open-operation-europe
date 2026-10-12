// The computer side's requests: supplies for a short objective, and equipment for an objective with room for it.
#pragma once

#include "types.hpp"

uint16_t opeu_ai_request_pending(uint8_t* ds, uint8_t slot);
uint16_t opeu_ai_config_kind_is(uint8_t* ds, uint8_t kind);
uint16_t opeu_ai_short_objective(uint8_t* ds);
uint16_t opeu_ai_request_resources(uint8_t* ds);
uint16_t opeu_ai_destination_for_type(uint8_t* ds, uint8_t type);
uint8_t  opeu_ai_short_category(uint8_t* ds);
uint8_t  opeu_ai_random_category(uint8_t* ds);
uint16_t opeu_ai_request_equipment(uint8_t* ds);
