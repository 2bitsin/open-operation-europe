// Distributes the allocated amounts over the units (or the objective), lets the player choose each unit's regimental
// headquarters, and finishes the allocation.
#include "units/allocdst.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/allocpnl.hpp"

#include "common/game.hpp"
#include "core/chief.hpp"
#include "io/listdlg.hpp"
#include "io/waitkey.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/capacity.hpp"
#include "units/elemlink.hpp"
#include "units/release.hpp"
#include "units/resagg.hpp"
#include "units/supply.hpp"
#include "units/unitname.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>

using namespace opeu::game;

namespace {
// The buffers the prompt and a unit's name are formatted into.
constexpr std::uint16_t PROMPT_BYTES{ 0x64 };
constexpr std::uint16_t NAME_BYTES{ 0x20 };

// Stored resources are capped at this amount.
constexpr std::int32_t RESOURCE_LIMIT{ 60000 };

// Copies an allocation row into a stock item, clearing its extra word.
auto Fill(Item& item, Item const& row) -> void {
  item.type     = row.type;
  item.quantity = row.quantity;
  item.extra    = 0;
}

// Ammunition, fuel and food summed over the parties, in 32 bits.
using Sums = std::array<std::int32_t, 3>;

auto Add(Sums& sums, std::uint16_t const (&resources)[3]) -> void {
  std::ranges::transform(sums, resources, sums.begin(), [](std::int32_t sum, std::uint16_t amount) { return sum + amount; });
}

// The allocation rows [begin, end) a party takes.
struct Rows {
  std::uint8_t begin;
  std::uint8_t end;
};

// Empties the unit and gives it a fresh element, with its type's averages, for each nonempty allocation row.
auto RebuildUnit(Game& game, std::uint16_t unit_near, Rows rows, std::uint8_t types) -> void {
  auto& data{ Data(game) };
  auto& unit{ Record<Unit>(data, unit_near) };
  while (unit.first) opeu_remove_element(game, unit_near, unit.first);
  opeu_release_side_elements(game, Near(data, data.alloc_elements), 0);
  std::uint8_t used{ 0 };
  for (auto const& slot : std::span{ data.alloc_slots }.subspan(rows.begin, rows.end - rows.begin) | std::views::filter([](Item const& row) { return row.quantity != 0; })) {
    auto const element_near{ data.alloc_elements[used++] };
    auto&      element{ Record<Element>(data, element_near) };
    element.type     = slot.type;
    element.quantity = slot.quantity;
    element.damage   = 0;
    opeu_alloc_element_values(game, element_near, types);
    opeu_insert_element(game, unit_near, element_near);
  }
}

// Fills the objective's six items from its allocation rows and the spare stock, clearing the items left over.
auto FillObjective(DataSegment& data, Rows rows) -> void {
  auto&        objective{ Record<Objective>(data, data.alloc_objective) };
  std::uint8_t used{ 0 };
  auto const   stocked{ [](Item const& row) { return row.quantity != 0; } };
  for (auto const& row : std::span{ data.alloc_slots }.subspan(rows.begin, rows.end - rows.begin) | std::views::filter(stocked)) Fill(objective.items[used++], row);
  for (auto const& row : std::span{ data.supply_spare }.first(data.tally_reject_count) | std::views::filter(stocked)) Fill(objective.items[used++], row);
  std::ranges::for_each(std::span{ objective.items }.subspan(used), [](Item& item) { item.quantity = 0; });
}

// The first unit keeps what it can carry; the second unit takes the rest and the two exchange supplies.
auto ShareWithUnit(Game& game, Sums const& sums, Resources const& capacities) -> void {
  auto& data{ Data(game) };
  auto& unit{ Record<Unit>(data, data.alloc_units[0]) };
  auto& other{ Record<Unit>(data, data.alloc_units[1]) };
  for (auto const [resource, sum, capacity] : std::views::zip(std::views::iota(0uz), sums, Figures(capacities))) {
    unit.resources[resource]  = sum > std::int32_t{ capacity } ? capacity : static_cast<std::uint16_t>(sum);
    other.resources[resource] = static_cast<std::uint16_t>(static_cast<std::uint16_t>(sum) - unit.resources[resource]);
  }
  opeu_supply_exchange_units(game, data.alloc_units[0], data.alloc_units[1]);
}

// The unit keeps what it can carry; the objective takes the rest, capped at 60000, and the two exchange supplies.
auto ShareWithObjective(Game& game, Sums const& sums, Resources const& capacities) -> void {
  auto& data{ Data(game) };
  auto& unit{ Record<Unit>(data, data.alloc_units[0]) };
  auto& objective{ Record<Objective>(data, data.alloc_objective) };
  for (auto const [resource, sum, capacity] : std::views::zip(std::views::iota(0uz), sums, Figures(capacities))) {
    unit.resources[resource] = sum > std::int32_t{ capacity } ? capacity : static_cast<std::uint16_t>(sum);
    if (sum - unit.resources[resource] > RESOURCE_LIMIT)
      objective.resources[resource] = RESOURCE_LIMIT;
    else
      objective.resources[resource] = static_cast<std::uint16_t>(static_cast<std::uint16_t>(sum) - unit.resources[resource]);
  }
  opeu_supply_exchange_objective(game, data.alloc_units[0], data.alloc_objective);
}
} // namespace

// Rebuilds each unit's element list from its allocation rows (or fills the objective's six items), then splits the
// summed resources: the first unit up to its capacities, the rest to the second unit or (capped at 60000) to the
// objective.
void opeu_alloc_distribute(Game& game, Allocation const& allocation) {
  auto&                    data{ Data(game) };
  Sums                     sums{};
  std::array<Resources, 2> capacities{};
  Rows                     rows{ 0, allocation.slots };
  for (auto const [party, unit_near] : std::views::zip(std::views::iota(0uz), data.alloc_units)) {
    if (unit_near != 0) {
      RebuildUnit(game, unit_near, rows, allocation.types);
      Add(sums, Record<Unit>(data, unit_near).resources);
      capacities[party] = UnitCapacities(data, unit_near);
      rows              = { rows.end, static_cast<std::uint8_t>(rows.end + allocation.other_slots) };
    } else {
      FillObjective(data, { rows.begin, static_cast<std::uint8_t>(rows.begin + allocation.spare_slots) });
      Add(sums, Record<Objective>(data, data.alloc_objective).resources);
      capacities[1] = ObjectiveCapacities(data, data.alloc_objective);
    }
  }
  if (data.alloc_units[1] != 0)
    ShareWithUnit(game, sums, capacities[0]);
  else
    ShareWithObjective(game, sums, capacities[0]);
}

// For each present unit, lists its elements in the choice dialog and asks for the regimental headquarters, formatting
// the prompt (100 bytes) and the unit's name (32 bytes) in the routine's buffers; the dialog's select callback is
// opeu_tally_mark.
void opeu_alloc_choose_hq(Game& game) {
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ PROMPT_BYTES + NAME_BYTES };
  auto const          name{ buffers.At(0) };
  auto const          text{ buffers.At(NAME_BYTES) };
  for (std::uint8_t i{ 0 }; i < 2; ++i) {
    auto const unit_near{ data.alloc_units[i] };
    if (unit_near == 0) continue;
    std::uint8_t listed{ 0 };
    for (auto it{ Elements(data, Record<Unit>(data, unit_near)).begin() }; it != std::default_sentinel; ++it) data.alloc_elements[listed++] = it.Near();
    data.choice_dialog.rows = static_cast<std::uint8_t>(listed + 1);
    SetListPredicate(data.choice_dialog, ListSelect::TALLY_MARK);
    opeu_request_format(game, std::array<std::uint16_t, 3>{ text, Near(data, data.regiment_hq_prompt), opeu_unit_name(game, unit_near, name) });
    opeu_ops_chief_text(game, text);
    opeu_list_dialog(game, Near(data, data.choice_dialog), 1, 9);
  }
}

void opeu_alloc_finish(Game& game) {
  opeu_pending_finish(game, 3);
}
