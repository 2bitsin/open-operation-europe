// The fixture's include cycle: a header whose Call reaches a routine declared in a header that includes this one.
#pragma once

#include "common/memory.hpp"

#include <cstdint>

enum RowsService : std::uint16_t {
  ROWS_FIRST,
};

uint16_t svc_rows(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);

struct Rows {
  std::uint16_t first;
};

inline auto RowsOf(std::uint8_t* ds) -> Rows {
  return { Call(ds, svc_rows, ROWS_FIRST) };
}
