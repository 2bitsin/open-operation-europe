// Element averages and quantity totals of units, divisions, sides and objectives. Totals wrap at 16 bits within a
// unit and a division (a side sums its divisions into 32 bits); counts wrap at 8 bits.
#include "units/stats.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "core/recindex.hpp"
#include "types.hpp"

#include <algorithm>
#include <ranges>
#include <span>
#include <type_traits>

using namespace opeu::game;

namespace {
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

namespace opeu::game::detail::stats {
namespace {
[[nodiscard]] auto InCategory(DataSegment const& data, std::uint8_t type, std::uint8_t category) -> bool {
  return category == ANY_CATEGORY || data.element_types[type].category == category;
}

// Adds one more holder's quantity to the totals.
constexpr auto ADD{ [](auto const& totals, Quantity const& more) -> std::remove_cvref_t<decltype(totals)> {
  return { static_cast<decltype(totals.total)>(totals.total + more.total), static_cast<std::uint8_t>(totals.count + more.count) };
} };
} // namespace

auto UnitQuantity(DataSegment const& data, std::uint16_t unit, std::uint8_t category) -> Quantity {
  auto const of_category{ [&](Element const& element) { return InCategory(data, element.type, category); } };
  auto const quantity{ [](Element const& element) { return Quantity{ element.quantity, 1 }; } };
  return std::ranges::fold_left(Elements(data, Record<Unit>(data, unit)) | std::views::filter(of_category) | std::views::transform(quantity), Quantity{ 0, 0 }, ADD);
}

auto GroupQuantity(DataSegment const& data, std::uint16_t group, std::uint8_t category) -> Quantity {
  auto const ready{ [](Unit const& unit) { return (unit.flags & Bits(UnitFlag::READY)) != 0; } };
  auto const quantity{ [&](Unit const& unit) { return UnitQuantity(data, Near(data, unit), category); } };
  auto const units{ GroupUnits(data, opeu_group_side(group), opeu_group_index(group)) };
  return std::ranges::fold_left(units | std::views::filter(ready) | std::views::transform(quantity), Quantity{ 0, 0 }, ADD);
}

auto SideQuantity(DataSegment const& data, std::uint8_t side, std::uint8_t category) -> WideQuantity {
  auto const ready{ [](Group const& group) { return (group.flags & Bits(GroupFlag::READY)) != 0; } };
  auto const quantity{ [&](Group const& group) { return GroupQuantity(data, Near(data, group), category); } };
  auto const groups{ std::span{ data.groups }.subspan(GROUPS_PER_SIDE * side, GROUPS_PER_SIDE) };
  return std::ranges::fold_left(groups | std::views::filter(ready) | std::views::transform(quantity), WideQuantity{ 0, 0 }, ADD);
}

auto ObjectiveQuantity(DataSegment const& data, std::uint16_t objective, std::uint8_t category) -> Quantity {
  auto const stocked{ [&](Item const& item) { return item.quantity && InCategory(data, item.type, category); } };
  auto const quantity{ [](Item const& item) { return Quantity{ item.quantity, 1 }; } };
  return std::ranges::fold_left(Record<Objective>(data, objective).items | std::views::filter(stocked) | std::views::transform(quantity), Quantity{ 0, 0 }, ADD);
}
} // namespace opeu::game::detail::stats

uint16_t opeu_unit_quantity(Game& game, uint16_t unit, uint8_t category, uint8_t* count) {
  auto const quantity{ UnitQuantity(Data(game), unit, category) };
  *count = quantity.count;
  return quantity.total;
}

uint32_t opeu_side_quantity(Game& game, uint8_t side, uint8_t category, uint8_t* count) {
  auto const quantity{ SideQuantity(Data(game), side, category) };
  *count = quantity.count;
  return quantity.total;
}

uint16_t opeu_objective_quantity(Game& game, uint16_t objective, uint8_t category, uint8_t* count) {
  auto const quantity{ ObjectiveQuantity(Data(game), objective, category) };
  *count = quantity.count;
  return quantity.total;
}
