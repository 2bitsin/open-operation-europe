// The fixture's initial code pointers.
#include "common/data-segment.hpp"

auto InitialActions(DataSegment& data) -> void {
  data.control_actions = { {
    { 0x0000, 0x0200 },
    { 0x0000, 0x0000 },
  } };
}

void opeu_raw_view(std::uint8_t* ds) {
  ds[4] = Data(ds).control_actions[0].offset;
}
