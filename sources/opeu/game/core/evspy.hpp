// The commander's evening visitor in scenario 4.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_spy_commander_ready(Game& game);
void     opeu_spy_portrait_text(Game& game, uint16_t text, uint16_t show);
void     opeu_spy_present_a(Game& game);
void     opeu_spy_show_a(Game& game);
void     opeu_spy_present_b(Game& game);
void     opeu_spy_approach(Game& game);
void     opeu_spy_event(Game& game);
