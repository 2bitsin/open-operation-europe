// Start-up: memory blocks, data files and the out-of-memory notice.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_memory_fail(Game& game, uint16_t name);
void opeu_alloc_segments(Game& game);
void opeu_open_data_files(Game& game);
void opeu_app_prepare(Game& game);
