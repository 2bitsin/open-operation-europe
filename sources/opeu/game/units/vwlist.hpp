// The information views' list builders and list row checks.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_list_side_objectives(Game& game, uint8_t side);
uint8_t opeu_list_unit_elements(Game& game, uint16_t unit);
uint8_t opeu_list_objective_stocks(Game& game, uint16_t objective);
uint8_t opeu_view_objective_ok(Game& game);
uint8_t opeu_view_false(Game& game);
uint8_t opeu_view_group_ok(Game& game);
uint8_t opeu_view_unit_ok(Game& game);
uint8_t opeu_view_false_2(Game& game);
uint8_t opeu_view_false_3(Game& game);
uint8_t opeu_view_true(Game& game);
uint8_t opeu_list_element_types(Game& game, uint16_t list, uint8_t mask, uint8_t category);
