#pragma once

#include "common/draw-request.hpp"
#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_deploy_true(Game& game);
void    opeu_deploy_draw_rows(Game& game, opeu::game::DrawRequest const& request);
