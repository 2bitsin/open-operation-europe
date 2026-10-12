// Expanded memory for the large graphics buffers.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_ems_map_pages(uint8_t* ds, HostEvent event, int16_t handle, int16_t count, int16_t first);
void    opeu_view_prepare(uint8_t* ds, HostEvent event);
void    opeu_ems_map_dialog(uint8_t* ds, HostEvent event);
void    opeu_gfx_wait(uint8_t* ds, HostEvent event);
void    opeu_display_prepare(uint8_t* ds, HostEvent event);
int16_t opeu_memory_release(uint8_t* ds, HostEvent event);
int16_t opeu_ems_reserve(uint8_t* ds, HostEvent event, uint16_t sp, uint16_t handle, int16_t pages);
int16_t opeu_ems_alloc_sprite(uint8_t* ds, HostEvent event, uint16_t sp);
int16_t opeu_ems_alloc_numdlg(uint8_t* ds, HostEvent event, uint16_t sp);
int16_t opeu_ems_alloc_tile(uint8_t* ds, HostEvent event, uint16_t sp);
int16_t opeu_ems_alloc_frame(uint8_t* ds, HostEvent event, uint16_t sp);
int16_t opeu_ems_detect(uint8_t* ds, HostEvent event, uint16_t sp);

// Runs the services the module's routines call through their event.
uint16_t svc_emsmap(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
