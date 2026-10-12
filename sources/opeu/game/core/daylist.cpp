// Dated records that are due, and an objective's level. A dated record is due when (year << 4) + month reaches its
// packed date byte; kind 255 matches every kind; bit 0 of a record's state excludes it.
#include "core/daylist.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint8_t ANY_KIND{ 255 };
constexpr std::uint8_t DONE{ 0x01 }; // DailyRecord::state: the record has happened
} // namespace

// Writes the indices of the due records whose flags meet mask to the byte list at the near pointer list; returns
// their number.
uint8_t opeu_list_due_records(uint8_t ds[65536], uint16_t list, uint8_t mask, uint8_t kind) {
  auto&         data{ Data(ds) };
  std::uint16_t cursor{ list };
  auto const    today{ (data.year << 4) + data.month };
  for (std::uint8_t index{ 0 }; auto const& record : data.daily_records) {
    if ((record.flags & mask) && (record.kind == kind || kind == ANY_KIND) && today >= record.date
        && !(record.state & DONE))
      Record<std::uint8_t>(data, cursor++) = index;
    ++index;
  }
  return static_cast<std::uint8_t>(cursor - list);
}

uint8_t opeu_objective_level(const uint8_t ds[65536], uint16_t objective) {
  return Record<Objective>(Data(ds), objective).flags & Bits(ObjectiveFlag::LEVEL_MASK);
}
