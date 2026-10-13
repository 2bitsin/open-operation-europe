// The allocation screen's type panel and the allocation arithmetic.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_alloc_clear_panel(Game& game);
void     opeu_alloc_type_panel(Game& game, uint8_t slot, uint16_t type);
uint16_t opeu_alloc_groups_nonempty(Game& game, uint8_t first, uint8_t second);
void     opeu_alloc_element_values(Game& game, uint16_t element, uint8_t count);
uint16_t opeu_alloc_scaled(Game& game, int32_t value, uint16_t amount, int32_t total);
