// The operations chief's portrait panel, and the messages, waits and number prompts shown beside it.
#pragma once

#include "common/events.hpp"
#include "common/text.hpp"
#include "types.hpp"

uint16_t opeu_ops_chief_show(Game& game);
void     opeu_ops_chief_text(Game& game, Text text);
uint16_t opeu_number_prompt(Game& game, Text text, uint16_t low, uint16_t high, uint16_t value, uint16_t d);
uint16_t opeu_ops_chief_number(Game& game, Text text, uint16_t low, uint16_t high, uint16_t value, uint16_t d);
uint16_t opeu_ops_chief_wait(Game& game, Text text);
