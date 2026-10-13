// The arrival of reserve units: every unit of a side marked arriving is placed on the map, its division and elements
// made ready, and the human side is shown the arrival.
#include "core/arrivals.hpp"
#include "core/people.hpp"
#include "types.hpp"
#include "units/orders.hpp"

#include "common/arrival.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/order.hpp"
#include "common/scenario.hpp"
#include "core/dialogs.hpp"
#include "io/sound.hpp"
#include "map/arrplace.hpp"
#include "map/maperase.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint8_t RANK_COMMANDER{ 3 };
} // namespace

void opeu_process_arrivals(Game& game, uint8_t side) {
  auto& data{ Data(game) };
  data.request_person = opeu_find_side_record(game, side, RANK_COMMANDER);
  bool arrived{ false };
  for (std::uint8_t g = 0; g < GROUPS_PER_SIDE; ++g) {
    auto& group{ GroupOf(data, side, g) };
    for (std::uint8_t u = 0; u < UNITS_PER_GROUP; ++u) {
      auto&      unit{ data.units[UNITS_PER_SIDE * side + UNITS_PER_GROUP * g + u] };
      auto const near{ Near(data, unit) };
      if (!(unit.flags & Bits(UnitFlag::ARRIVING))) continue;
      if (!opeu_arrival_locate(game, Near(data, unit.x), Near(data, unit.y), side)) return;
      for (auto& task : group.tasks) task.kind = Bits(TaskKind::NONE);
      group.flags &= ~Bits(GroupFlag::RESERVE);
      group.flags |= Bits(GroupFlag::READY);
      unit.flags  &= ~Bits(UnitFlag::ARRIVING);
      unit.flags  |= Bits(UnitFlag::READY);
      opeu_clear_unit_orders(game, near, ALL_ORDER_SLOTS);
      for (auto& element : Elements(data, unit)) {
        element.flags &= ~Bits(ElementFlag::ARRIVING);
        element.flags |= Bits(ElementFlag::READY);
      }
      opeu_arrival_prepare(game, near);
      if (data.side_status[side] == SIDE_PLAYED) {
        opeu_set_turn_context(game, ARRIVAL_CONTEXT);
        opeu_arrival_location(game, unit.x, unit.y, side);
        opeu_arrival_view(game, near);
        data.arrival_side = side;
        data.arrival_unit = near;
        opeu_daily_show(game, 0, side, Presenter::ARRIVAL);
        opeu_finish_side_initialization(game, 0);
      }
      arrived = true;
    }
    if (arrived) break;
  }
  data.side_flags[side] &= ~Bits(SideFlag::RESERVES_PENDING);
}

// The hourly arrivals: in scenario 3 the second side's reserves arrive only in weather 0 or 1.
void opeu_hourly_c_stage_c(Game& game) {
  auto& data{ Data(game) };
  for (std::uint8_t side = 0; side < SIDE_COUNT; ++side)
    if (data.side_flags[side] & Bits(SideFlag::RESERVES_PENDING)) {
      if (CurrentScenario(data) == Scenario::D_DAY && side == 1 && data.weather != 0 && data.weather != 1) continue;
      opeu_process_arrivals(game, side);
    }
}
