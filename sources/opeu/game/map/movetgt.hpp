#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_move_adjacent(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y);
uint16_t opeu_move_objective_target(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y, uint8_t side);
uint16_t opeu_move_unit_target(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y);
uint16_t opeu_move_level_raise(uint8_t* ds, uint16_t record, uint8_t value);
uint16_t opeu_move_level_lower(uint8_t* ds, uint16_t record, uint8_t value);

// The movement routines' shared services (opeu_move_unit and this module raise them); pointer words are DS offsets.
enum class MoveService : uint16_t {
  RATING           = 0,  // the current unit's movement rating
  GRID_STEP        = 1,  // x*, y*, direction
  COST             = 2,  // x, y, unit
  DISTANCE         = 3,  // x, y, x2, y2
  DESTINATION      = 4,  // unit, x*, y*
  REACHABLE        = 5,  // x, y, tx, ty, unit
  IS_ENGINEER      = 6,  // element category code
  STEP_TOWARD      = 7,  // x*, y*, tx, ty
  CLEAR_PATH       = 8,  // x, y
  FIND_OBJECTIVE   = 9,  // x, y
  OBJECTIVE_SIDE   = 10, // objective
  FIND_UNIT        = 11, // x, y
  UNIT_SIDE        = 12, // unit
  CELL_SIDE        = 13, // x, y
  SUPPLIED         = 14, //
  ARRIVAL_LOCATION = 15, // x, y, side
  ARRIVAL_VIEW     = 16, // unit
  UNIT_ERASE       = 17, // unit
  ARRIVAL_PREPARE  = 18, // unit
  CLAMP            = 19, //
  CELL_MARKED      = 20, // x, y, side
  MINES            = 21, // x, y
  SPOT             = 22, //
  CONSUME          = 23, //
  ENEMY_ADJACENT   = 24, //
  RESTORE_ORDER    = 25, // unit
  UNIT_ELIGIBLE    = 26, // unit
  UNIT_SPOTTED     = 27, // unit
  UNIT_NAME        = 28, // unit, buffer
  FORMAT           = 29, // destination, format, value, side name, unit name
  UNIT_MESSAGE     = 30, // unit, text
};

// The services of this module.
uint16_t svc_map_move(uint8_t* ds, const char* module, uint16_t id, const uint16_t* args, uint16_t count);
uint16_t svc_movetgt(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
