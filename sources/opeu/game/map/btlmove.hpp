#pragma once

#include "common/events.hpp"
#include "map/btlelem.hpp"
#include "map/grid-cell.hpp"
#include "types.hpp"

// Moves the element between two cells of the field: redraws the cell it leaves and draws it at the other.
void     opeu_battle_move_element(Game& game, opeu::game::ShownElement shown, opeu::game::CellMove move);
void     opeu_battle_remove_unit(Game& game, uint16_t unit, uint16_t element);
void     opeu_battle_remove_list(Game& game, uint16_t unit);
uint8_t  opeu_battle_attack_rating(Game& game, uint16_t attacker, uint16_t target);
uint16_t opeu_battle_best_target(Game& game, uint16_t element);
uint16_t opeu_battle_can_act(Game& game, uint16_t element);
void     opeu_battle_update_element(Game& game, uint16_t element);
