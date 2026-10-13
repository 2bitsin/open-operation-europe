// Per-type tallies of unit elements and objective stock with weighted averages, for a unit/objective or unit/unit
// comparison. Quantities sum into 16-bit words, weighted values into 32-bit accumulators; averages are signed 32-bit
// quotients truncated to bytes. Stock items that do not fit the requested class go to a separate list.
#include "units/tally.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/recindex.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/unitname.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <span>

using namespace opeu::game;

namespace {
// One tally contribution: a type's quantity, its second amount and its three quantity-weighted values.
struct TallyEntry {
  std::uint8_t  type;
  std::uint16_t quantity;
  std::uint16_t second;
  std::uint32_t morale;
  std::uint32_t training;
  std::uint32_t experience;
};

// The row of the first `count` tally rows holding the type, or `count` when none does.
auto RowOf(DataSegment const& data, std::uint8_t count, std::uint8_t type) -> std::uint8_t {
  auto const first{ data.supply_types.begin() };
  return static_cast<std::uint8_t>(std::ranges::find(first, first + count, type) - first);
}

// Starts the row for a type with no quantity and no second amount; its sums keep what they held.
auto OpenRow(DataSegment& data, std::uint8_t row, std::uint8_t type) -> void {
  data.supply_types[row]   = type;
  data.supply_amounts[row] = 0;
  data.supply_others[row]  = 0;
}

// Empties the row's weighted sums.
auto ClearSums(DataSegment& data, std::uint8_t row) -> void {
  data.supply_morale_sums[row]     = 0;
  data.supply_training_sums[row]   = 0;
  data.supply_experience_sums[row] = 0;
}

// Adds the entry to the row and its quantity to the total.
auto AddToRow(DataSegment& data, std::uint8_t row, TallyEntry const& entry) -> void {
  data.supply_total                += entry.quantity;
  data.supply_amounts[row]         += entry.quantity;
  data.supply_others[row]          += entry.second;
  data.supply_morale_sums[row]     += entry.morale;
  data.supply_training_sums[row]   += entry.training;
  data.supply_experience_sums[row] += entry.experience;
}

// The records a tally compares: a unit against another unit or against an objective's stock.
struct Comparison {
  std::uint16_t unit;
  std::uint16_t other;
  std::uint16_t source;
};

// Records the compared records and names the unit in the first heading.
auto StartTally(Game& game, Comparison const& comparison) -> void {
  auto& data{ Data(game) };
  data.tally_unit       = comparison.unit;
  data.tally_other_unit = comparison.other;
  data.tally_source     = comparison.source;
  opeu_unit_name(game, comparison.unit, Near(data, data.tally_first_name));
}
} // namespace

// Selects the element of the list row the cursor is on.
uint8_t opeu_tally_mark(Game& game) {
  auto& data{ Data(game) };
  Record<Element>(data, data.tally_records[data.list_row]).flags |= Bits(ElementFlag::SELECTED);
  return 1;
}

// Merges every element of the unit into the tallies; returns the new row count.
uint8_t opeu_tally_elements(Game& game, uint16_t unit, uint8_t count) {
  auto& data{ Data(game) };
  for (auto& element : Elements(data, Record<Unit>(data, unit))) {
    auto const quantity{ element.quantity };
    auto const row{ RowOf(data, count, element.type) };
    if (row == count) {
      OpenRow(data, row, element.type); // a new row adds to the sums a former tally left
      ++count;
    }
    AddToRow(data, row,
             TallyEntry{
                 .type       = element.type,
                 .quantity   = quantity,
                 .second     = element.damage,
                 .morale     = std::uint32_t{ element.morale } * quantity,
                 .training   = std::uint32_t{ element.training } * quantity,
                 .experience = std::uint32_t{ element.experience } * quantity,
             });
  }
  return count;
}

// Tallies the objective's stock items of the requested class: kind 0-4 accepts categories 0-4, kind 5 only 5, kind 6+
// only 6+. The other items go to the reject list.
uint8_t opeu_tally_stock(Game& game, uint16_t source_offset, uint8_t count, uint8_t kind) {
  auto& data{ Data(game) };
  auto& source{ Record<Objective>(data, source_offset) };
  data.tally_reject_count = 0;
  for (auto const& item : source.items) {
    if (!item.quantity) continue;
    auto const category{ data.element_types[item.type].category };
    if ((kind <= 4 && category <= 4) || (kind == 5 && category == 5) || (kind >= 6 && category >= 6)) {
      std::uint32_t const fill{ static_cast<std::uint32_t>(opeu_min16(99, opeu_mul_div(item.quantity, 100, data.request_capacities[category] * 3))) * item.quantity };
      std::uint32_t const strength{ source.strength * static_cast<std::uint32_t>(item.quantity) };
      std::uint32_t const condition{ opeu_min16(99, 100 - opeu_mul_div(source.strength, 100, source.strength_limit)) * static_cast<std::uint32_t>(item.quantity) };
      auto const          row{ RowOf(data, count, item.type) };
      if (row == count) {
        OpenRow(data, row, item.type);
        ClearSums(data, row);
        ++count;
      }
      AddToRow(data, row, TallyEntry{ .type = item.type, .quantity = item.quantity, .second = item.extra, .morale = fill, .training = strength, .experience = condition });
    } else {
      auto& reject{ data.supply_spare[data.tally_reject_count] };
      reject.type     = item.type;
      reject.quantity = item.quantity;
      reject.extra    = item.extra;
      ++data.tally_reject_count;
    }
  }
  return count;
}

// Per-type averages, dividing each sum by max(quantity, 1). See opeu_supply_count.
void opeu_tally_average(Game& game, uint8_t count) {
  auto&      data{ Data(game) };
  auto const average{ [&](auto const& sums, auto& averages) {
    std::ranges::transform(std::span{ sums }.first(count), data.supply_amounts, averages.begin(),
                           [](std::uint32_t sum, std::uint16_t amount) { return static_cast<std::uint8_t>(static_cast<std::int32_t>(sum) / static_cast<std::int32_t>(opeu_max16(amount, 1))); });
  } };
  average(data.supply_morale_sums, data.supply_morale_averages);
  average(data.supply_training_sums, data.supply_training_averages);
  average(data.supply_experience_sums, data.supply_experience_averages);
}

// A unit against an objective's stock; the unit's slot selects the stock kind.
uint8_t opeu_tally_unit_stock(Game& game, uint16_t unit, uint16_t source_offset) {
  StartTally(game, Comparison{ .unit = unit, .other = 0, .source = source_offset });
  opeu_string_copy(game, Near(Data(game), Data(game).tally_second_name), source_offset);
  uint8_t kind;
  if (opeu_category_6(opeu_unit_index(unit)))
    kind = 5;
  else if (opeu_category_7(opeu_unit_index(unit)))
    kind = 6;
  else
    kind = 4;
  auto const count{ opeu_tally_stock(game, source_offset, opeu_tally_elements(game, unit, 0), kind) };
  opeu_tally_average(game, count);
  return count;
}

// Two units' elements merged into one tally.
uint8_t opeu_tally_units(Game& game, uint16_t unit, uint16_t other) {
  StartTally(game, Comparison{ .unit = unit, .other = other, .source = 0 });
  opeu_unit_name(game, other, Near(Data(game), Data(game).tally_second_name));
  auto const count{ opeu_tally_elements(game, other, opeu_tally_elements(game, unit, 0)) };
  opeu_tally_average(game, count);
  return count;
}
