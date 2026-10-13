#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_location_blast(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y, uint8_t side);
void opeu_blast_erase(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y);
void opeu_blast_pause(uint8_t* ds, HostEvent event);

// The map segment's shared service kinds (svc_map_plan): argument words in the order of each callee.
enum class MapService : uint16_t {
  DRAW_FAR         = 0,  // x, y, w, h, mode, segment, offset
  DRAW_NEAR        = 1,  // x, y, w, h, mode, DS offset
  RESTORE_NEAR     = 2,  // x, y, w, h, mode, DS offset
  SAVE_NEAR        = 3,  // x, y, w, h, DS offset
  GLYPH_NEAR       = 4,  // x, y, w, h, color, DS offset
  LINE             = 5,  // x1, y1, x2, y2, color
  RESTORE_FAR      = 6,  // x, y, w, h, mode, segment, offset
  DRAW_MASKED_NEAR = 7,  // x, y, w, h, mode, DS offset
  SPRITE_COMPOSE   = 8,  // x, y, w, h, image offset, segment, mask offset, segment, buffer offset, segment, flags
  BLIT_FAR         = 9,  // x, y, w, h, mode, segment, offset
  PATTERN_NEAR     = 10, // x, y, w, h, mode, DS offset
  SCREEN_SHIFT     = 11, // x1, y1, x2, y2
  WAIT             = 12, // ticks
  CONTROL_MESSAGE  = 13, // text, person
  STATUS_MODE      = 14, // mode
  READ_FAR_BYTES   = 16, // source offset, source segment, DS destination, size
};

// The services of this module and the map segment's shared ones.
uint16_t svc_blast(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
uint16_t svc_map_plan(uint8_t* ds, const char* module, uint16_t kind, const uint16_t* args, uint16_t count);
