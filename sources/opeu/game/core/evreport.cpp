// The hourly unit reports and the second hourly event round. Each hour the ready regiments of both sides that can
// act and hold an order other than rest are checked: one whose battalions average above 90 (rested) and one that
// carries weight but has no fuel has its saved order restored and, on a watched side, is reported to the player.
#include "core/evreport.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/evangel.hpp"
#include "core/evbomb.hpp"
#include "core/evcomms.hpp"
#include "core/evloss.hpp"
#include "core/evpartis.hpp"
#include "core/evspy.hpp"
#include "core/evunits.hpp"
#include "core/evvisit.hpp"
#include "core/evwound.hpp"
#include "core/msglook.hpp"
#include "core/recindex.hpp"
#include "core/strings.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/capacity.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// opeu_report_present as the far routine (segment, offset) the daily item's presenter calls.
struct FarRoutine {
  std::uint16_t segment;
  std::uint16_t offset;
};
constexpr FarRoutine REPORT_PRESENTER{ 0x108c, 0x1302 };
// The code segment the angel event's actions are stored with, and the bomb event's load.
constexpr std::uint16_t ANGEL_CODE_SEGMENT{ 0x108c };
constexpr std::uint16_t BOMB_LOAD{ 0 };

constexpr std::uint16_t TEXT{ offsetof(DataSegment, event_buffer) };
constexpr std::uint16_t RESTED_MESSAGE{ 0x3b };
constexpr std::uint16_t NO_FUEL_MESSAGE{ 0x3c };
constexpr std::uint16_t RESTED_ITEM{ 0x13e };  // the daily item of the first rested report
constexpr std::uint16_t NO_FUEL_ITEM{ 0x156 }; // and of the first out-of-fuel report
constexpr std::uint16_t RESTED_AVERAGE{ 90 };

// Reports each listed regiment with the message; the first report opens the daily item.
void ReportUnits(Game& game, ReportList const& list, std::uint16_t message, std::uint16_t item) {
  bool seen{ false };
  for (auto const unit : list) {
    if (!unit) continue;
    opeu_string_copy(game, TEXT, opeu_message_lookup(game, svc_msglook, static_cast<int16_t>(message)));
    opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), unit, TEXT);
    if (!seen) {
      auto const side{ static_cast<std::uint8_t>(opeu_unit_side(unit)) };
      opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), item, side, static_cast<std::uint32_t>(REPORT_PRESENTER.segment) << 16 | REPORT_PRESENTER.offset);
      seen = true;
    }
  }
}
} // namespace

void opeu_report_present(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(4));
}

void opeu_report_units_a(Game& game, ReportList const& list) {
  ReportUnits(game, list, RESTED_MESSAGE, RESTED_ITEM);
}

void opeu_report_units_b(Game& game, ReportList const& list) {
  ReportUnits(game, list, NO_FUEL_MESSAGE, NO_FUEL_ITEM);
}

// The lists are cleared once, before the first side, so the second side's report repeats the first side's
// entries in the slots it does not fill itself.
void opeu_report_event(Game& game) {
  auto&      data{ Data(game) };
  ReportList rested{};
  ReportList no_fuel{};
  for (std::uint8_t side{ 0 }; side < 2; ++side) {
    for (std::uint8_t group{ 0 }; group < GROUPS_PER_SIDE; ++group) {
      if (GroupOf(data, side, group).flags & Bits(GroupFlag::AI_MARK)) continue;
      auto const units{ GroupUnits(data, side, group) };
      for (std::uint8_t slot{ 0 }; slot < UNITS_PER_GROUP; ++slot) {
        auto&      unit{ units[slot] };
        auto const near{ Near(data, unit) };
        if (!(unit.flags & Bits(UnitFlag::READY)) || !opeu_unit_selection_eligible(game, near)) continue;
        auto const order{ unit.orders[0].kind };
        if (order == Kind(UnitOrder::REST) || order == ORDER_UNUSED) continue;
        if (opeu_unit_average11(game, near) > RESTED_AVERAGE) {
          rested[group * UNITS_PER_GROUP + slot] = near;
          opeu_restore_unit_order(game, near);
        }
        std::uint16_t capacity_a{};
        std::uint16_t weight{};
        std::uint16_t capacity_c{};
        opeu_unit_capacities(game, near, &capacity_a, &weight, &capacity_c);
        if (weight && !unit.resources[1]) {
          no_fuel[group * UNITS_PER_GROUP + slot] = near;
          opeu_restore_unit_order(game, near);
        }
      }
    }
    if (data.side_status[side] == SIDE_PLAYED) {
      opeu_report_units_a(game, rested);
      opeu_report_units_b(game, no_fuel);
    }
  }
}

// The second round of hourly events, with the unit reports between the visit and the wound events.
void opeu_turn_hourly_b(Game& game) {
  opeu_daily_context(game, static_cast<uint8_t>(Data(game).current_side));
  opeu_partisan_event(game);
  opeu_loss_event_a(game);
  opeu_loss_event_b(game);
  opeu_bomb_event(game, BOMB_LOAD);
  opeu_angel_event(game, ANGEL_CODE_SEGMENT);
  opeu_spy_event(game);
  opeu_visit_event(game);
  opeu_report_event(game);
  opeu_wound_event(game);
  opeu_objective_comms_event(game);
  opeu_unit_hourly_event(game);
}

auto port_routine_opeu_report_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_report_present(game);
  return 0;
}
