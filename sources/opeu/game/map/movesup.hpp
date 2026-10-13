#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_move_supplied(Game& game);
void     opeu_move_consume(Game& game);
void     opeu_move_sighted(Game& game, uint16_t unit, uint16_t other);
