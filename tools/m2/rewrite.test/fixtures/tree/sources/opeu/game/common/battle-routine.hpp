// The fixture's battle keys: linear routine addresses the link table resolves.
#pragma once

#include <cstdint>

enum class BattleRoutine : std::uint32_t {
  GRID_STEP   = 0x01000,
  SET_ORDER   = 0x01010,
  RANDOM      = 0x01020,
  UNLINKED    = 0x01030,
};
