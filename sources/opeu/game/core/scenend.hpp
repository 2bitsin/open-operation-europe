// The end of a scenario, the far-image accessors of the scenario copy and the shutdown helpers.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_image_byte(Game const& game, const uint8_t* image, uint16_t p);
uint16_t opeu_image_word(Game const& game, const uint8_t* image, uint16_t p);
uint32_t opeu_image_pointer(Game const& game, uint16_t p);
uint16_t opeu_end_unit_strength(Game& game, HostEvent event);
uint16_t opeu_end_objective_points(Game& game, HostEvent event);
void     opeu_end_palette_dark(Game& game, HostEvent event);
void     opeu_end_palette_show(Game& game, HostEvent event);
void     opeu_scenario_end(Game& game, HostEvent event);
void     opeu_free_error(Game& game, HostEvent event, uint16_t name);
void     opeu_free_segments(Game& game, HostEvent event);
void     opeu_close_data_files(Game& game, HostEvent event);
void     opeu_app_finish(Game& game, HostEvent event);

// The services of this module in the native build.
uint16_t svc_scenend(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
