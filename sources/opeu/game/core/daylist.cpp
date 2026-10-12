// The equipment types in service, and an objective's level. A type is in service when (year << 4) + month reaches its
// packed date byte; kind 255 matches every category; the paratroop types are never listed.
#include "core/daylist.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint8_t ANY_KIND{ 255 };
} // namespace

// Writes the indices of the types in service that the nations in mask field to the byte list at the near pointer
// list; returns their number.
uint8_t opeu_list_due_records(uint8_t ds[65536], uint16_t list, uint8_t mask, uint8_t kind) {
  auto&         data{ Data(ds) };
  std::uint16_t cursor{ list };
  auto const    today{ (data.year << 4) + data.month };
  for (std::uint8_t index{ 0 }; auto const& type : data.element_types) {
    if ((type.sides & mask) && (type.category == kind || kind == ANY_KIND) && today >= type.date && !(type.flags & Bits(TypeFlag::AIRBORNE))) Record<std::uint8_t>(data, cursor++) = index;
    ++index;
  }
  return static_cast<std::uint8_t>(cursor - list);
}

uint8_t opeu_objective_level(const uint8_t ds[65536], uint16_t objective) {
  return Record<Objective>(Data(ds), objective).flags & Bits(ObjectiveFlag::LEVEL_MASK);
}
