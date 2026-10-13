// The hourly condition of every ready regiment of both sides: lost communications may come back and an enemy spy may
// be caught (each reported), the regiment eats into its food, and each battalion's fatigue and morale move with its
// orders and supplies before the regiment's morale check.
#include "core/evunits.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/stats.hpp"
#include "units/unitrout.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t COMMS_TEXT{ offsetof(DataSegment, comms_unit_text) };
constexpr std::uint16_t SPY_TEXT{ offsetof(DataSegment, spy_unit_text) };
constexpr std::uint8_t  CATEGORIES{ 8 };
constexpr std::uint8_t  ORDER_DONE{ 100 }; // Order::progress of an order carried out

auto Rand(Game& game, unsigned bound) -> std::uint16_t {
  return game.State().RandomBounded(static_cast<std::uint16_t>(bound));
}
} // namespace

// The side's level is the delivery level for a player's side, else 2; it sets the odds and the fatigue of resting.
void opeu_unit_hourly_event(Game& game) {
  auto& data{ Data(game) };
  for (std::uint8_t side{ 0 }; side < 2; ++side) {
    std::uint16_t const level{ DeliveryLevel(data, side) };
    for (std::uint8_t group{ 0 }; group < GROUPS_PER_SIDE; ++group) {
      auto const units{ GroupUnits(data, side, group) };
      for (std::uint8_t slot{ 0 }; slot < UNITS_PER_GROUP; ++slot) {
        auto&      unit{ units[slot] };
        auto const near{ Near(data, unit) };
        if (!(unit.flags & Bits(UnitFlag::READY))) continue;
        if (!(unit.flags & Bits(UnitFlag::COMMUNICATIONS)) && Rand(game, level * 10u + 24u) == 6) {
          unit.flags |= Bits(UnitFlag::COMMUNICATIONS);
          opeu_unit_report(game, near, COMMS_TEXT);
        }
        if ((unit.flags & Bits(UnitFlag::SPOTTED)) && Rand(game, (level + 5u) * 20u) == 8) {
          unit.flags &= static_cast<std::uint8_t>(~Bits(UnitFlag::SPOTTED));
          opeu_unit_report(game, near, SPY_TEXT);
        }
        for (std::uint8_t category{ 0 }; category < CATEGORIES; ++category) {
          auto const    quantity{ UnitQuantity(data, near, category).total };
          std::uint16_t food{ unit.resources[2] };
          food              = SubWord(food, opeu_max16(quantity / opeu_max16(data.quantity_divisors[category], 1), 1));
          unit.resources[2] = food;
        }
        auto const& food{ unit.resources[2] };
        for (auto& element : Elements(data, unit.first)) {
          auto const order{ unit.orders[0].kind };
          if (order == ORDER_UNUSED)
            element.experience = SubByte(element.experience, Rand(game, 5u - level) + 3);
          else if (unit.orders[0].progress >= ORDER_DONE)
            element.experience = AddByte(element.experience, 1, RATING_LIMIT);
          else if (order == Kind(UnitOrder::REST) && food != 0) {
            element.experience = SubByte(element.experience, Rand(game, (5u - level) * 2u) + 8);
            element.morale     = SubByte(element.morale, 1);
          } else
            element.experience = AddByte(element.experience, 2, RATING_LIMIT);
          if (food == 0) {
            element.experience = AddByte(element.experience, 2, RATING_LIMIT);
            element.morale     = SubByte(element.morale, 1);
          } else if (element.morale >= element.experience)
            element.morale = AddByte(element.morale, Rand(game, 3) + 1, RATING_LIMIT);
          else
            element.morale = SubByte(element.morale, Rand(game, 3) + 2);
          if (data.element_types[element.type].fuel_economy != 0 && unit.resources[1] == 0) element.morale = SubByte(element.morale, 1);
          if (unit.resources[0] == 0) element.morale = SubByte(element.morale, 1);
          if (element.morale < opeu_unit_average9(game, near))
            element.morale = AddByte(element.morale, 1, RATING_LIMIT);
          else
            element.morale = SubByte(element.morale, 1);
        }
        opeu_unit_morale_check(game, side, group, slot);
      }
    }
  }
}
