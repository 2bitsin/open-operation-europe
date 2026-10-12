// The end of a scenario, the far-image accessors of the scenario copy and the shutdown helpers.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_image_byte(const uint8_t* ds, const uint8_t* image, uint16_t p);
uint16_t opeu_image_word(const uint8_t* ds, const uint8_t* image, uint16_t p);
uint32_t opeu_image_pointer(const uint8_t* ds, uint16_t p);
uint16_t opeu_end_unit_strength(uint8_t* ds, HostEvent event);
uint16_t opeu_end_objective_points(uint8_t* ds, HostEvent event);
void     opeu_end_palette_dark(uint8_t* ds, HostEvent event);
void     opeu_end_palette_show(uint8_t* ds, HostEvent event);
void     opeu_scenario_end(uint8_t* ds, HostEvent event, uint16_t sp);
void     opeu_free_error(uint8_t* ds, HostEvent event, uint16_t name);
void     opeu_free_segments(uint8_t* ds, HostEvent event);
void     opeu_close_data_files(uint8_t* ds, HostEvent event);
void     opeu_app_finish(uint8_t* ds, HostEvent event, uint16_t sp);

// The services of this module in the native build.
uint16_t svc_scenend(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
