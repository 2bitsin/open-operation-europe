// The fixture's battle keys: linear routine addresses the link table resolves.
#pragma once

#include <cstdint>

enum class BattleRoutine : std::uint32_t {
  GRID_STEP = 0x01000,
  SET_ORDER = 0x01010,
  RANDOM    = 0x01020,
  UNLINKED  = 0x01030,
  PATH_STEP = 0x01040,
  PATH_MARK = 0x01050,
  ASK       = 0x01060,
  MARK_CELL = 0x01070,
};
