// The paratroop drop: counting it down, flying it, and calling it off.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_para_clear(Game& game);
uint16_t opeu_para_ready(Game& game);
uint16_t opeu_para_drop_due(Game& game);
void     opeu_para_drop_present(Game& game);
void     opeu_para_drop_event(Game& game);
