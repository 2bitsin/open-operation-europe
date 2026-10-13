// Releases the current side's elements and regiments, and formats percentages. A record is released unless it carries
// a status bit outside the caller's allowed mask; released records' near pointers are appended to an optional list.
#include "units/release.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/value.hpp"

using namespace opeu::game;

namespace {
// The status bits of Element::flags and Unit::flags that keep a record from being released unless the mask allows them.
constexpr std::uint8_t ELEMENT_STATUS_BITS{ 0x3c };
constexpr std::uint8_t UNIT_STATUS_BITS{ 0x5c };
// The mask that releases only the regiments of the line slots (categories 0-5).
constexpr std::uint8_t LINE_ONLY_MASK{ 0x10 };

// Appends a near pointer to the word list at `list` (a DS offset), when there is one.
auto Append(Game& game, std::uint16_t list, std::uint8_t index, std::uint16_t near) -> void {
  if (list) SetWord(game, static_cast<std::uint16_t>(list + index * 2u), near);
}
} // namespace

uint8_t opeu_release_side_elements(Game& game, uint16_t list, uint8_t mask) {
  auto&        data{ Data(game) };
  std::uint8_t released{ 0 };
  for (std::uint8_t i{ 0 }; i < ELEMENTS_PER_SIDE; ++i) {
    auto& element{ data.elements[data.current_side * ELEMENTS_PER_SIDE + i] };
    if (element.flags & ~mask & ELEMENT_STATUS_BITS) continue;
    element.next       = 0;
    element.quantity   = 0;
    element.flags      = 0;
    element.experience = 0;
    element.training   = 0;
    element.morale     = 0;
    Append(game, list, released, Near(data, element));
    ++released;
  }
  return released;
}

uint8_t opeu_release_side_units(Game& game, uint16_t list, uint8_t mask) {
  auto&        data{ Data(game) };
  std::uint8_t released{ 0 };
  for (std::uint8_t group{ 0 }; group < GROUPS_PER_SIDE; ++group) {
    auto const division{ data.current_side * GROUPS_PER_SIDE + group };
    if (!(data.groups[division].flags & Bits(GroupFlag::READY))) continue;
    for (std::uint8_t slot{ 0 }; slot < UNITS_PER_GROUP; ++slot) {
      auto& unit{ data.units[division * UNITS_PER_GROUP + slot] };
      if ((unit.flags & ~mask & UNIT_STATUS_BITS) || (mask == LINE_ONLY_MASK && !opeu_category_0_to_3(slot) && !opeu_category_4_or_5(slot))) continue;
      unit.first = 0;
      unit.y     = 255;
      unit.x     = 255;
      opeu_clear_unit_orders(game, Near(data, unit), 3);
      unit.flags = 0;
      Append(game, list, released, Near(data, unit));
      ++released;
    }
  }
  return released;
}
