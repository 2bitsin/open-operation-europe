// Unit supplies lost with destroyed battle elements.
//
// When an element's quantity drops from `before` to its current count, the unit loses the share of its fuel,
// ammunition or food that the lost vehicles carried: each element weighs quantity x its type's fuel use or its
// category's ammunition or food weight, and the lost part is weight x lost / 100 of the unit's stock per percent.
#include "map/btlspend.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "core/numeric.hpp"
#include "map/btllist.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {
enum class Supply : uint8_t {
  AMMUNITION = 0,
  FUEL       = 1,
  FOOD       = 2
};

// The weight an element of this type puts on the supply.
[[nodiscard]] auto Weight(DataSegment const& data, Element const& element, Supply supply) -> uint16_t {
  auto const& type{ data.element_types[element.type] };
  switch (supply) {
  case Supply::AMMUNITION: return data.ammunition_capacities[type.category];
  case Supply::FOOD:       return data.food_capacities[type.category];
  default:                 return type.fuel_capacity;
  }
}

void Spend(DataSegment& data, uint16_t unit, uint16_t element, uint16_t before, Supply supply) {
  auto&    record{ Record<Unit>(data, unit) };
  auto&    stock{ record.resources[std::to_underlying(supply)] };
  uint16_t total{ 0 };
  for (auto it{ Elements(data, record).begin() }; it != std::default_sentinel; ++it)
    total = static_cast<uint16_t>(total + Weight(data, *it, supply) * (element == it.Near() ? before : (*it).quantity));
  auto const  share{ opeu_mul_div(stock, 100, total) };
  auto const& lost{ Record<Element>(data, element) };
  uint16_t    value{ stock };
  value = SubWord(value, opeu_mul_div(static_cast<uint16_t>(Weight(data, lost, supply) * static_cast<uint16_t>(before - lost.quantity)), share, 100));
  stock = value;
}
} // namespace

void opeu_battle_spend_fuel(Game& game, uint16_t element, uint16_t before) {
  auto& data{ Data(game) };
  if (!Weight(data, Record<Element>(data, element), Supply::FUEL)) return;
  auto const unit{ opeu_battle_element_unit(game, element) };
  bool       fuelled{ false };
  for (auto const& other : Elements(data, Record<Unit>(data, unit)))
    if (Weight(data, other, Supply::FUEL) && other.quantity) {
      fuelled = true;
      break;
    }
  if (!fuelled) {
    Record<Unit>(data, unit).resources[std::to_underlying(Supply::FUEL)] = 0;
    return;
  }
  Spend(data, unit, element, before, Supply::FUEL);
}

void opeu_battle_spend_ammo(Game& game, uint16_t element, uint16_t before) {
  Spend(Data(game), opeu_battle_element_unit(game, element), element, before, Supply::AMMUNITION);
}

void opeu_battle_spend_stores(Game& game, uint16_t element, uint16_t before) {
  Spend(Data(game), opeu_battle_element_unit(game, element), element, before, Supply::FOOD);
}
