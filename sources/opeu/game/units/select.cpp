// Selection recovery for divisions, units and elements: select the first ready one, or the preferred element.
#include "units/select.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "core/recindex.hpp"
#include "types.hpp"

#include <algorithm>
#include <ranges>
#include <span>

using namespace opeu::game;

namespace {
template <typename Record, typename Flag> [[nodiscard]] auto IsReady(Record const& record, Flag ready) -> bool {
  return (record.flags & Bits(ready)) != 0;
}
} // namespace

// Selects the side's first ready division; keeps its other flags and any other selection.
void opeu_select_first_group(Game& game, uint8_t side) {
  auto&      data{ Data(game) };
  auto const groups{ std::span{ data.groups }.subspan(GROUPS_PER_SIDE * side, GROUPS_PER_SIDE) };
  if (auto const group{ std::ranges::find_if(groups, [](Group const& group) { return IsReady(group, GroupFlag::READY); }) }; group != groups.end()) group->flags |= Bits(GroupFlag::SELECTED);
}

// Selects the division's first ready unit.
void opeu_select_first_unit(Game& game, uint16_t group) {
  auto const units{ GroupUnits(Data(game), opeu_group_side(group), opeu_group_index(group)) };
  if (auto const unit{ std::ranges::find_if(units, [](Unit const& unit) { return IsReady(unit, UnitFlag::READY); }) }; unit != units.end()) unit->flags |= Bits(UnitFlag::SELECTED);
}

// Selects the unit's element of the lowest category rank; ties keep the first element.
void opeu_select_preferred_element(Game& game, uint16_t unit) {
  struct Choice {
    std::uint8_t  rank;
    std::uint16_t element; // 0 while no element ranks below 255
  };
  auto&      data{ Data(game) };
  auto const better{ [&](Choice best, Element const& element) {
    auto const rank{ data.select_category_ranks[data.element_types[element.type].category] };
    return rank < best.rank ? Choice{ rank, Near(data, element) } : best;
  } };
  auto const preferred{ std::ranges::fold_left(Elements(data, Record<Unit>(data, unit)), Choice{ 255, 0 }, better) };
  if (preferred.element) Record<Element>(data, preferred.element).flags |= Bits(ElementFlag::SELECTED);
}
