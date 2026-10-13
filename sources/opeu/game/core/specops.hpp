// The Special Forces missions: raid, sabotage, cut communications, assassination and reconnaissance.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <cstdint>

// The services the Special Forces command (core/specfrc) and its missions call through their event.
enum class SpecialService : std::uint16_t {
  REQUEST_FORMAT       = 0,  // (destination, format, arguments...)
  CONTROL_MESSAGE      = 1,  // (text, person)
  CONFIRM_MESSAGE      = 2,  // (text, person) -> yes
  MESSAGE_LOOKUP       = 3,  // (id) -> near pointer to the message
  ARMY_PRINT           = 4,  // (format, arguments...)
  PENDING_FINISH       = 5,  // (value)
  DAILY_SHOW           = 6,  // (id, side, presenter offset, presenter segment)
  FIND_OBJECTIVE_AT    = 7,  // (x, y) -> objective
  DELAY_DIALOG         = 8,  // (text, last, person, menu) -> choice, 0xffff when cancelled
  ARMY_TITLE           = 9,  // (text)
  MAP_PICK_LOCATION    = 10, // (near x, near y) -> picked
  OBJECTIVE_SIDE       = 11, // (objective) -> side
  ARMY_POSITION        = 12, // (x, y)
  DAILY_CONTEXT        = 13, // (side)
  FAR_ACTION           = 14, // (segment, offset): runs a mission
  FIND_UNIT_AT         = 15, // (x, y) -> unit
  UNIT_SIDE            = 16, // (unit) -> side
  UNIT_SPECIAL         = 17, // (unit) -> whether the unit is a special unit
  UNIT_NAME            = 18, // (unit, buffer) -> buffer
  CELL_INFLUENCE_SIDE  = 19, // (x, y) -> side
  FIND_PERSON_TEMPLATE = 20, // (mask, kind) -> person
};

// Order::kind of a side's special order: the mission and whether it targets a city or a unit.
enum class SpecialMission : std::uint8_t {
  RAID_CITY     = 0,
  RAID_UNIT     = 1,
  SABOTAGE      = 2, // a city's depot
  CUT_COMM_CITY = 3,
  CUT_COMM_UNIT = 4,
  ASSASSINATE   = 5, // the commander of a unit
  RECON_CITY    = 6,
  RECON_UNIT    = 7,
  NONE          = 0xff, // no mission under way
};

void opeu_special_present(Game& game);
void opeu_special_raid(Game& game);
void opeu_special_sabotage(Game& game);
void opeu_special_cut_comm(Game& game);
void opeu_special_assassinate(Game& game);
void opeu_special_recon(Game& game);

// The services of the Special Forces command and its missions as direct calls.
uint16_t svc_specops(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
