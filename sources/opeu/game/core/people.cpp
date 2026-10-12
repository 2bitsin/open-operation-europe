// Finding a side's general by staff role or by division.
#include "types.hpp"
#include "core/people.hpp"
#include "data-segment.hpp"

#include <span>

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// The side's 20 generals.
auto SideGenerals(DataSegment const& data, uint8_t side) {
  return std::span{ data.people }.subspan(side * PEOPLE_PER_SIDE,
                                          PEOPLE_PER_SIDE);
}

} // namespace

// Near pointer of the side's first general whose staff role is kind; 0 when none is.
uint16_t opeu_find_side_record(const uint8_t ds[65536], uint8_t side,
                               uint8_t kind) {
  auto const& data{ Data(ds) };
  for (auto const& person : SideGenerals(data, side))
    if (person.status != PERSON_VACANT &&
        (person.role & Bits(PersonRole::ROLE_MASK)) == kind)
      return Near(data, person);
  return 0;
}

// Near pointer of the side's general commanding division group; 0 when none does.
uint16_t opeu_find_group_record(const uint8_t ds[65536], uint8_t side,
                                uint8_t group) {
  auto const& data{ Data(ds) };
  for (auto const& person : SideGenerals(data, side)) {
    uint8_t const assignment{ person.assignment };
    if (person.status != PERSON_VACANT &&
        (assignment & Bits(Assignment::DIVISION)) &&
        (assignment & Bits(Assignment::DIVISION_MASK)) == group)
      return Near(data, person);
  }
  return 0;
}

auto port_routine_opeu_find_group_record(std::uint8_t*        ds,
                                         std::uint16_t const* args,
                                         unsigned count) -> std::uint32_t {
  return opeu_find_group_record(ds, (uint8_t)Arg(args, count, 0),
                                (uint8_t)Arg(args, count, 1));
}

auto port_routine_opeu_find_side_record(std::uint8_t*        ds,
                                        std::uint16_t const* args,
                                        unsigned count) -> std::uint32_t {
  return opeu_find_side_record(ds, (uint8_t)Arg(args, count, 0),
                               (uint8_t)Arg(args, count, 1));
}
