// Requests against the records: the objective slot a delivery fills, the marks of a requested regiment.
#pragma once

#include "types.hpp"

uint16_t opeu_request_slot(uint8_t* ds, uint16_t objective, uint8_t type);
void     opeu_request_complete(uint8_t* ds, uint8_t side);
uint16_t opeu_request_group_ready(uint8_t* ds, uint8_t side);
