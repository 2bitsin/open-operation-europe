// Turn action 3: transfer of command.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_transfer_hours(Game& game, uint16_t from, uint16_t to, uint16_t person);
uint8_t  opeu_transfer_group_enabled(Game& game);
uint8_t  opeu_transfer_unit_enabled(Game& game);
uint8_t  opeu_transfer_element_enabled(Game& game);
uint8_t  opeu_transfer_units_fill(Game& game);
uint8_t  opeu_transfer_elements_fill(Game& game);
void     opeu_transfer_present(Game& game);
void     opeu_turn_action_3(Game& game);
