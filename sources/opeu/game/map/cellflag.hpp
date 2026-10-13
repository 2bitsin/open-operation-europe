#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

void     opeu_mark_cell(Game& game, opeu::game::GridCell cell, uint8_t side);
void     opeu_unmark_cell(Game& game, opeu::game::GridCell cell, uint8_t side);
uint16_t opeu_cell_marked(Game& game, opeu::game::GridCell cell, uint8_t side);
int16_t  opeu_cell_influence(Game& game, opeu::game::GridCell cell);
uint8_t  opeu_cell_influence_side(Game& game, opeu::game::GridCell cell);
