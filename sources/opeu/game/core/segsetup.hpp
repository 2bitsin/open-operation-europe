// Start-up: memory blocks, data files and the out-of-memory notice.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

void opeu_memory_fail(uint8_t* ds, HostEvent event, uint16_t name);
void opeu_alloc_segments(uint8_t* ds, HostEvent event, uint16_t sp);
void opeu_open_data_files(uint8_t* ds, HostEvent event);
void opeu_app_prepare(uint8_t* ds, HostEvent event, uint16_t sp);

// The services the start-up routines are called with.
uint16_t svc_segsetup(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
