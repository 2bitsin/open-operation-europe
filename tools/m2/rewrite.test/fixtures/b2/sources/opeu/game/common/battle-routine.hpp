// The fixture's battle keys.
#pragma once

#include <cstdint>

enum class BattleRoutine : std::uint32_t {
  STEP = 0x01000,
  MARK = 0x01010,
  SHOW = 0x01020,
};
