// The Special Forces missions: raid, sabotage, cut communications, assassination and reconnaissance.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <cstdint>

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
