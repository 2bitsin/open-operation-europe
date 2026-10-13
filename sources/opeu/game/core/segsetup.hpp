// Start-up: memory blocks, data files and the out-of-memory notice.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_memory_fail(Game& game, HostEvent event, uint16_t name);
void opeu_alloc_segments(Game& game, HostEvent event);
void opeu_open_data_files(Game& game, HostEvent event);
void opeu_app_prepare(Game& game, HostEvent event);

// The services the start-up routines are called with.
uint16_t svc_segsetup(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
