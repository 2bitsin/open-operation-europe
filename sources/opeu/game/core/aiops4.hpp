// The computer side's requests: supplies for a short objective, and equipment for an objective with room for it.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_ai_request_pending(Game& game, uint8_t slot);
uint16_t opeu_ai_config_kind_is(Game& game, uint8_t kind);
uint16_t opeu_ai_short_objective(Game& game);
uint16_t opeu_ai_request_resources(Game& game);
uint16_t opeu_ai_destination_for_type(Game& game, uint8_t type);
uint8_t  opeu_ai_short_category(Game& game);
uint8_t  opeu_ai_random_category(Game& game);
uint16_t opeu_ai_request_equipment(Game& game);
