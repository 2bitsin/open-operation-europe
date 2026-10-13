// Element averages and quantity totals of units, divisions, sides and objectives. Totals wrap at 16 bits within a
// unit and a division (a side sums its divisions into 32 bits); counts wrap at 8 bits.
#include "units/stats.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "core/recindex.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
inline constexpr std::uint8_t ANY_CATEGORY{ 255 };

// The mean of one byte field over the unit's elements, 0 for an empty unit.
auto ElementAverage(Game const& game, std::uint16_t unit, std::uint8_t Element::* field) -> std::uint16_t {
  auto const&   data{ Data(game) };
  std::uint16_t count{ 0 };
  std::uint16_t sum{ 0 };
  for (auto const& element : Elements(data, Record<Unit>(data, unit))) {
    sum += element.*field;
    ++count;
  }
  if (!count) return 0;
  return sum / count;
}
} // namespace

// Whether the unit or one of its elements carries the special mark.
uint16_t opeu_unit_special(Game& game, uint16_t unit) {
  auto&       data{ Data(game) };
  auto const& record{ Record<Unit>(data, unit) };
  if (record.flags & Bits(UnitFlag::SPOTTED)) return 1;
  for (auto const& element : Elements(data, record))
    if (element.flags & Bits(ElementFlag::SPOTTED)) return 1;
  return 0;
}

uint16_t opeu_unit_average9(Game& game, uint16_t unit) {
  return ElementAverage(game, unit, &Element::morale);
}

uint16_t opeu_unit_average10(Game& game, uint16_t unit) {
  return ElementAverage(game, unit, &Element::training);
}

uint16_t opeu_unit_average11(Game& game, uint16_t unit) {
  return ElementAverage(game, unit, &Element::experience);
}

// The quantity of the unit's elements of a category (255: any) and how many elements that is.
uint16_t opeu_unit_quantity(Game& game, uint16_t unit, uint8_t category, uint8_t* count) {
  auto&         data{ Data(game) };
  std::uint16_t total{ 0 };
  *count = 0;
  for (auto const& element : Elements(data, Record<Unit>(data, unit)))
    if (category == ANY_CATEGORY || data.element_types[element.type].category == category) {
      total += element.quantity;
      ++*count;
    }
  return total;
}

// The same over the ready units of a division.
uint16_t opeu_group_quantity(Game& game, uint16_t group, uint8_t category, uint8_t* count) {
  auto&         data{ Data(game) };
  auto const    first{ UNITS_PER_SIDE * opeu_group_side(group) + UNITS_PER_GROUP * opeu_group_index(group) };
  std::uint16_t total{ 0 };
  *count = 0;
  for (std::uint8_t index{ 0 }; index < UNITS_PER_GROUP; ++index) {
    auto const& unit{ data.units[first + index] };
    if (unit.flags & Bits(UnitFlag::READY)) {
      std::uint8_t unit_count;
      total  += opeu_unit_quantity(game, Near(data, unit), category, &unit_count);
      *count += unit_count;
    }
  }
  return total;
}

// The same over the ready divisions of a side.
uint32_t opeu_side_quantity(Game& game, uint8_t side, uint8_t category, uint8_t* count) {
  auto&         data{ Data(game) };
  std::uint32_t total{ 0 };
  *count = 0;
  for (std::uint8_t index{ 0 }; index < GROUPS_PER_SIDE; ++index) {
    auto const& group{ GroupOf(data, side, index) };
    if (group.flags & Bits(GroupFlag::READY)) {
      std::uint8_t group_count;
      total  += opeu_group_quantity(game, Near(data, group), category, &group_count);
      *count += group_count;
    }
  }
  return total;
}

// The quantity an objective stocks of a category (255: any) and how many items that is.
uint16_t opeu_objective_quantity(Game& game, uint16_t objective, uint8_t category, uint8_t* count) {
  auto&         data{ Data(game) };
  std::uint16_t total{ 0 };
  *count = 0;
  for (auto const& item : Record<Objective>(data, objective).items)
    if (item.quantity && (category == ANY_CATEGORY || data.element_types[item.type].category == category)) {
      total += item.quantity;
      ++*count;
    }
  return total;
}
