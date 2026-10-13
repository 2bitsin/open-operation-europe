// Requests against the records: the objective slot a delivery fills, the marks of a requested regiment.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_request_slot(Game& game, uint16_t objective, uint8_t type);
void     opeu_request_complete(Game& game, uint8_t side);
uint16_t opeu_request_group_ready(Game& game, uint8_t side);
