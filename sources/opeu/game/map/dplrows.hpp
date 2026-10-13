#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_deploy_true(Game& game);
void    opeu_deploy_draw_rows(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);
