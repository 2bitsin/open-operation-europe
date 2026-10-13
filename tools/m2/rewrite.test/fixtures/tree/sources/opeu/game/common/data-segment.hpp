// The fixture's data segment: one far code table.
#pragma once

#include <array>
#include <cstdint>

struct FarPointer {
  std::uint16_t offset;
  std::uint16_t segment;
};

struct DataSegment {
  std::array<FarPointer, 2> control_actions; // the routine of each command
  std::array<FarPointer, 1> message_texts;   // data, not code
};

inline auto Data(std::uint8_t* ds) -> DataSegment& {
  return *reinterpret_cast<DataSegment*>(ds);
}
