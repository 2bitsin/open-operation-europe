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
#include "common/random.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/stats.hpp"
#include "units/unitrout.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The service the event calls through its event.
enum Service : std::uint16_t {
  UNIT_MESSAGE = 7, // (unit, text)
};

constexpr std::uint16_t COMMS_TEXT{ offsetof(DataSegment, comms_unit_text) };
constexpr std::uint16_t SPY_TEXT{ offsetof(DataSegment, spy_unit_text) };
constexpr std::uint8_t  CATEGORIES{ 8 };
constexpr std::uint8_t  ORDER_DONE{ 100 }; // Order::progress of an order carried out

auto Rand(Game& game, unsigned bound) -> std::uint16_t {
  return RandomBelow(game, static_cast<std::uint16_t>(bound));
}
} // namespace

// The side's level is the delivery level for a player's side, else 2; it sets the odds and the fatigue of resting.
void opeu_unit_hourly_event(Game& game, HostEvent event) {
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
          Call(game, event, UNIT_MESSAGE, near, COMMS_TEXT);
        }
        if ((unit.flags & Bits(UnitFlag::SPOTTED)) && Rand(game, (level + 5u) * 20u) == 8) {
          unit.flags &= static_cast<std::uint8_t>(~Bits(UnitFlag::SPOTTED));
          Call(game, event, UNIT_MESSAGE, near, SPY_TEXT);
        }
        for (std::uint8_t category{ 0 }; category < CATEGORIES; ++category) {
          std::uint8_t  count{};
          auto const    quantity{ opeu_unit_quantity(game, near, category, &count) };
          std::uint16_t food{ unit.resources[2] };
          opeu_sub_word(opeu_max16(quantity / opeu_max16(data.quantity_divisors[category], 1), 1), &food);
          unit.resources[2] = food;
        }
        auto const& food{ unit.resources[2] };
        for (auto& element : Elements(data, unit.first)) {
          auto const order{ unit.orders[0].kind };
          if (order == ORDER_UNUSED)
            opeu_sub_byte(Rand(game, 5u - level) + 3, &element.experience);
          else if (unit.orders[0].progress >= ORDER_DONE)
            opeu_add_byte(1, RATING_LIMIT, &element.experience);
          else if (order == Kind(UnitOrder::REST) && food != 0) {
            opeu_sub_byte(Rand(game, (5u - level) * 2u) + 8, &element.experience);
            opeu_sub_byte(1, &element.morale);
          } else
            opeu_add_byte(2, RATING_LIMIT, &element.experience);
          if (food == 0) {
            opeu_add_byte(2, RATING_LIMIT, &element.experience);
            opeu_sub_byte(1, &element.morale);
          } else if (element.morale >= element.experience)
            opeu_add_byte(Rand(game, 3) + 1, RATING_LIMIT, &element.morale);
          else
            opeu_sub_byte(Rand(game, 3) + 2, &element.morale);
          if (data.element_types[element.type].fuel_economy != 0 && unit.resources[1] == 0) opeu_sub_byte(1, &element.morale);
          if (unit.resources[0] == 0) opeu_sub_byte(1, &element.morale);
          if (element.morale < opeu_unit_average9(game, near))
            opeu_add_byte(1, RATING_LIMIT, &element.morale);
          else
            opeu_sub_byte(1, &element.morale);
        }
        opeu_unit_morale_check(game, side, group, slot);
      }
    }
  }
}

uint16_t svc_evunits(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case UNIT_MESSAGE: opeu_unit_report(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  default:           port_unhandled("evunits", id, count);
  }
  return 0;
}

auto port_routine_opeu_unit_hourly_event(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_unit_hourly_event(game, svc_evunits);
  return 0;
}
