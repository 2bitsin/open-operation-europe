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
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// Adds quantity, second amount and weighted values to the tally row `row`, or starts a new row at `count`.
auto TallyInto(DataSegment& data, std::uint8_t& count, std::uint8_t type, std::uint16_t quantity, std::uint16_t second, std::uint32_t sum_a, std::uint32_t sum_b, std::uint32_t sum_c,
               bool accumulate_new) -> void {
  std::uint8_t row{ 0 };
  while (row < count && data.supply_types[row] != type) ++row;
  data.supply_total += quantity;
  if (row != count) {
    data.supply_amounts[row]         += quantity;
    data.supply_others[row]          += second;
    data.supply_morale_sums[row]     += sum_a;
    data.supply_training_sums[row]   += sum_b;
    data.supply_experience_sums[row] += sum_c;
    return;
  }
  data.supply_types[count]   = type;
  data.supply_amounts[count] = quantity;
  data.supply_others[count]  = second;
  if (accumulate_new) {
    data.supply_morale_sums[count]     += sum_a;
    data.supply_training_sums[count]   += sum_b;
    data.supply_experience_sums[count] += sum_c;
  } else {
    data.supply_morale_sums[count]     = sum_a;
    data.supply_training_sums[count]   = sum_b;
    data.supply_experience_sums[count] = sum_c;
  }
  ++count;
}

// Records the compared records and names the unit in the first heading.
auto StartTally(Game& game, std::uint16_t unit, std::uint16_t other, std::uint16_t source) -> void {
  auto& data{ Data(game) };
  data.tally_unit       = unit;
  data.tally_other_unit = other;
  data.tally_source     = source;
  opeu_unit_name(game, unit, Near(data, data.tally_first_name));
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
    TallyInto(data, count, element.type, quantity, element.damage, std::uint32_t{ element.morale } * quantity, std::uint32_t{ element.training } * quantity,
              std::uint32_t{ element.experience } * quantity, true);
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
      TallyInto(data, count, item.type, item.quantity, item.extra, fill, strength, condition, false);
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
  auto& data{ Data(game) };
  for (std::uint8_t row{ 0 }; row < count; ++row) {
    auto const divisor{ static_cast<std::int32_t>(opeu_max16(data.supply_amounts[row], 1)) };
    data.supply_morale_averages[row]     = static_cast<std::int32_t>(data.supply_morale_sums[row]) / divisor;
    data.supply_training_averages[row]   = static_cast<std::int32_t>(data.supply_training_sums[row]) / divisor;
    data.supply_experience_averages[row] = static_cast<std::int32_t>(data.supply_experience_sums[row]) / divisor;
  }
}

// A unit against an objective's stock; the unit's slot selects the stock kind.
uint8_t opeu_tally_unit_stock(Game& game, uint16_t unit, uint16_t source_offset) {
  StartTally(game, unit, 0, source_offset);
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
  StartTally(game, unit, other, 0);
  opeu_unit_name(game, other, Near(Data(game), Data(game).tally_second_name));
  auto const count{ opeu_tally_elements(game, other, opeu_tally_elements(game, unit, 0)) };
  opeu_tally_average(game, count);
  return count;
}
