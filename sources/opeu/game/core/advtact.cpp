// The staff's tactical advice: the officer checks the current side's situation in a fixed order and gives the first
// advice that applies, written into the tactics topic's buffer.
#include "core/advtact.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "core/grid-neighbours.hpp"
#include "types.hpp"

#include "common/advice.hpp"
#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/advenemy.hpp"
#include "core/daylist.hpp"
#include "core/freepers.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "core/sideutil.hpp"
#include "map/cellflag.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "units/aggpct.hpp"
#include "units/capacity.hpp"
#include "units/orders.hpp"
#include "units/ratio.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint16_t CHEAP_UNIT{ 300 }; // a unit worth at most this is weak
constexpr std::uint16_t LOW_SUPPLY{ 20 };  // percent
constexpr std::uint8_t  GOOD_RATING{ 50 };
constexpr std::uint8_t  NO_REQUEST{ 0xff };
constexpr std::uint16_t NO_ADVICE{ 0xffff };
// The flags of a unit that is not idle: ready, set aside for a drop, or arriving.
constexpr std::uint8_t BUSY{ Bits(UnitFlag::READY) | Bits(UnitFlag::AIRBORNE) | Bits(UnitFlag::ARRIVING) };

auto Tactics(Game& game, std::uint8_t const* far_text) -> Advisor {
  return { game, far_text, AdviceTopic::TACTICS };
}

// Calls visit(division, slot, unit) for each ready unit of the side's active divisions that can act.
template <typename Visit>
void ForEligibleUnits(Game& game, Visit&& visit) {
  auto&      data{ Data(game) };
  auto const side{ data.current_side };
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    if (!IsActiveGroup(GroupOf(data, side, g))) continue;
    auto const units{ GroupUnits(data, side, g) };
    for (std::uint8_t k{ 0 }; k < UNITS_PER_GROUP; ++k)
      if ((units[k].flags & Bits(UnitFlag::READY)) && opeu_unit_selection_eligible(game, Near(data, units[k])))
        if (visit(g, k, units[k])) return;
  }
}
} // namespace

// A city-level objective: held by a defending side with an enemy special fighting unit next to it, the officer urges
// its defense; held by the enemy under the attacking side's influence with a unit of ours next to it, the officer
// sees the chance to take it.
uint16_t opeu_advice_tactical_objective_threat(Game& game, const uint8_t* far_text) {
  auto&      data{ Data(game) };
  auto const advisor{ Tactics(game, far_text) };
  auto const side{ data.current_side };
  for (auto const& objective : data.objectives) {
    auto const near{ Near(data, objective) };
    if (!objective.active || opeu_objective_level(game, near) < CITY_LEVEL) continue;
    bool const ours{ opeu_objective_side(game, near) == side };
    if (ours ? data.side_stance[side] != 0 : data.side_stance[side] != Bits(Stance::OFFENSIVE)) continue;
    if (!ours && opeu_cell_influence_side(game, { objective.x, objective.y }) != side) continue;
    for (auto const [x, y] : CellsAround({ objective.x, objective.y })) {
      if (opeu_cell_side(game, x, y) != (ours ? opeu_opposite_side(side) : side)) continue;
      auto const unit{ opeu_find_unit_at(game, x, y) };
      if (!(ours ? opeu_unit_special(game, unit) : opeu_unit_selection_eligible(game, unit))) continue;
      if (!opeu_category_0_to_5(opeu_unit_index(unit))) continue;
      return advisor.SayFormatted(ours ? AdviceText::DEFEND_OBJECTIVE : AdviceText::TAKE_OBJECTIVE, near);
    }
  }
  return 0;
}

// A free general and a division led by a deputy: appoint a commander. A tired commander: leave it to the division
// commander. The selected unit outnumbered by enemy special units around it: one time in three, move headquarters.
uint16_t opeu_advice_tactical_command(Game& game, const uint8_t* far_text) {
  auto&      data{ Data(game) };
  auto const advisor{ Tactics(game, far_text) };
  auto const side{ data.current_side };
  if (opeu_find_free_person(game, side))
    for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
      auto const person{ opeu_find_group_record(game, side, g) };
      if (person && (Record<Person>(data, person).role & Bits(PersonRole::DEPUTY))) return advisor.SayFormatted(AdviceText::APPOINT_COMMANDER, g + 1);
    }
  auto const& commander{ Record<Person>(data, data.context_record1) };
  if (commander.health < opeu_percent(commander.max_health, 30)) return advisor.Say(AdviceText::LEAVE_TO_COMMANDER);
  auto const&  selected{ Record<Unit>(data, opeu_find_side_selected_unit(game, side)) };
  std::uint8_t own{ 1 };
  std::uint8_t enemy{ 0 };
  for (auto const [x, y] : CellsAround({ selected.x, selected.y })) {
    auto const unit{ opeu_find_unit_at(game, x, y) };
    if (!unit || !opeu_category_0_to_5(opeu_unit_index(unit))) continue;
    if (opeu_cell_side(game, x, y) == side)
      ++own;
    else if (opeu_unit_special(game, unit))
      ++enemy;
  }
  if (enemy > own && advisor.Random(3) == 1) return advisor.Say(AdviceText::MOVE_HEADQUARTERS);
  return 0;
}

// An attacking side with at least twelve cheap units and five short of a supply: one time in three, reorganize.
uint16_t opeu_advice_tactical_reorganize(Game& game, const uint8_t* far_text) {
  auto&      data{ Data(game) };
  auto const advisor{ Tactics(game, far_text) };
  if (data.side_stance[data.current_side] != Bits(Stance::OFFENSIVE)) return 0;
  std::uint8_t cheap{ 0 };
  std::uint8_t short_of_supply{ 0 };
  ForEligibleUnits(game, [&](std::uint8_t, std::uint8_t, Unit& unit) {
    auto const near{ Near(data, unit) };
    if (opeu_unit_value(game, near) <= CHEAP_UNIT) ++cheap;
    auto const capacity{ Figures(UnitCapacities(data, near)) };
    if (opeu_resource_percent(unit.resources[0], capacity[0]) <= LOW_SUPPLY || opeu_resource_percent(unit.resources[1], capacity[1]) <= LOW_SUPPLY
        || opeu_resource_percent(unit.resources[2], capacity[2]) <= LOW_SUPPLY)
      ++short_of_supply;
    return false;
  });
  if (cheap >= 12 && short_of_supply >= 5 && advisor.Random(3) == 1) return advisor.Say(AdviceText::REORGANIZE);
  return 0;
}

// For each unit of ours, in three passes: outnumbered by enemy special units around it (with chance 1/(6 - g)), send
// help; a fighting unit without an order inside our influence, send it to the front; a cheap fighting unit outside
// it, pull back. Each of the last two one time in three.
uint16_t opeu_advice_tactical_unit_deployment(Game& game, const uint8_t* far_text) {
  auto&         data{ Data(game) };
  auto const    advisor{ Tactics(game, far_text) };
  auto const    side{ data.current_side };
  std::uint16_t given{ 0 };
  for (std::uint8_t pass{ 0 }; pass < 3 && !given; ++pass) {
    ForEligibleUnits(game, [&](std::uint8_t g, std::uint8_t k, Unit& unit) {
      auto const   near{ Near(data, unit) };
      std::uint8_t own{ 1 };
      std::uint8_t enemy{ 0 };
      for (auto const [x, y] : CellsAround({ unit.x, unit.y })) {
        auto const other{ opeu_find_unit_at(game, x, y) };
        if (!other || !opeu_unit_special(game, other) || !opeu_category_0_to_5(opeu_unit_index(other))) continue;
        if (opeu_cell_side(game, x, y) == side)
          ++own;
        else
          ++enemy;
      }
      std::uint16_t text{ NO_ADVICE };
      if (pass == 0 && enemy > own && advisor.Random(static_cast<std::uint16_t>(6 - g)) == 1) {
        text = std::to_underlying(AdviceText::SEND_HELP);
      } else {
        auto const influence{ opeu_cell_influence_side(game, { unit.x, unit.y }) };
        auto const ordered{ UnitOrderDestination(game, near).valid };
        if (pass == 1 && side == influence && opeu_category_4_or_5(k) && !ordered && advisor.Random(3) == 1)
          text = std::to_underlying(AdviceText::SEND_TO_FRONT);
        else if (pass == 2 && side != influence && opeu_category_4_or_5(k) && opeu_unit_value(game, near) <= CHEAP_UNIT && advisor.Random(3) == 1)
          text = std::to_underlying(AdviceText::PULL_BACK);
      }
      if (text == NO_ADVICE) return false;
      given = advisor.SayAboutUnit(static_cast<AdviceText>(text), near);
      return true;
    });
  }
  return given;
}

// Whether the side's request slot holds a pending request.
uint16_t opeu_side_objective_slot_pending(Game& game, const uint8_t*, uint8_t slot) {
  auto& data{ Data(game) };
  return Record<Side>(data, data.current_side_ptr).requests[slot].kind != NO_REQUEST;
}

// Whether the side's request configuration is of `kind`.
uint16_t opeu_side_request_kind_is(Game& game, const uint8_t*, uint8_t kind) {
  auto const& data{ Data(game) };
  return data.request_configs[data.current_side].kind == kind;
}

// Below 20 percent of a supply with no airlift and no request under way: one time in three, request supplies.
// Otherwise an active division with fewer than four ready units: one time in three, reinforce it.
uint16_t opeu_advice_tactical_supply_reinforce(Game& game, const uint8_t* far_text) {
  auto&      data{ Data(game) };
  auto const advisor{ Tactics(game, far_text) };
  auto const side{ data.current_side };
  auto const percent{ Figures(SideResourcePercentages(game, side)) };
  if ((percent[0] < LOW_SUPPLY || percent[1] < LOW_SUPPLY || percent[2] < LOW_SUPPLY) && !Record<Side>(data, data.current_side_ptr).lift.planes && !opeu_side_request_kind_is(game, far_text, 0)
      && !opeu_side_objective_slot_pending(game, far_text, 0) && advisor.Random(3) == 1)
    return advisor.Say(AdviceText::REQUEST_SUPPLIES);
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    if (!IsActiveGroup(GroupOf(data, side, g))) continue;
    std::uint8_t ready{ 0 };
    for (auto const& unit : GroupUnits(data, side, g))
      if (unit.flags & Bits(UnitFlag::READY)) ++ready;
    if (ready < 4 && advisor.Random(3) == 1) return advisor.SayFormatted(AdviceText::REINFORCE_DIVISION, g + 1);
  }
  return 0;
}

// A defending side none of whose divisions' engineers (unit slot 6) is laying, clearing or bridging: one time in
// three, plant mines.
uint16_t opeu_advice_tactical_land_mines(Game& game, const uint8_t* far_text) {
  auto&        data{ Data(game) };
  auto const   advisor{ Tactics(game, far_text) };
  auto const   side{ data.current_side };
  std::uint8_t working{ 0 };
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    if (!IsActiveGroup(GroupOf(data, side, g))) continue;
    auto const& engineers{ GroupUnits(data, side, g)[6] };
    if (!(engineers.flags & Bits(UnitFlag::READY)) || !opeu_unit_selection_eligible(game, Near(data, engineers))) continue;
    auto const kind{ engineers.orders[0].kind };
    if (kind >= Bits(UnitOrder::LAY_MINES) && kind <= 7) ++working;
  }
  if (!data.side_stance[side] && !working && advisor.Random(3) == 1) return advisor.Say(AdviceText::PLANT_MINES);
  return 0;
}

// Whether a ready division has an idle unit of a fighting category. A division is skipped by the low bit of its first
// unit's second order slot, not by the division's reserve flag.
uint8_t opeu_ready_group_has_idle_unit(Game& game, const uint8_t*) {
  auto&      data{ Data(game) };
  auto const side{ data.current_side };
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    if (!(GroupOf(data, side, g).flags & Bits(GroupFlag::READY))) continue;
    auto const units{ GroupUnits(data, side, g) };
    if (units[0].orders[1].target & 1) continue;
    for (std::uint8_t k{ 0 }; k < UNITS_PER_GROUP; ++k)
      if (!(units[k].flags & BUSY) && (opeu_category_0_to_3(k) || opeu_category_4_or_5(k))) return 1;
  }
  return 0;
}

// Strong morale, clear weather and an able intelligence chief: with strike planes, or spare transports while the
// drop or the airlift is idle, one time in three ask the air force for help. At night with an able operations chief
// and no side order: call in the special forces. Strong morale, clear weather, a very able intelligence chief and an
// idle fighting unit: suggest a paratroop attack.
uint16_t opeu_advice_tactical_air_special_forces(Game& game, const uint8_t* far_text) {
  auto&         data{ Data(game) };
  auto const    advisor{ Tactics(game, far_text) };
  auto const&   side{ Record<Side>(data, data.current_side_ptr) };
  std::uint16_t spare{ side.planes };
  spare = SubWord(spare, side.lift.planes + side.drop.transports);
  auto const intelligence{ Record<Person>(data, data.context_record3).charm };
  if (side.strength >= GOOD_RATING && !data.weather && intelligence >= GOOD_RATING && (side.bombers || (spare && (!side.drop.transports || !side.lift.planes))) && advisor.Random(3) == 1)
    return advisor.Say(AdviceText::ASK_AIR_FORCE);
  if (opeu_hour_is_night(data.hour) && Record<Person>(data, data.context_record2).charm >= GOOD_RATING && side.order.kind == Bits(UnitOrder::NONE) && advisor.Random(3) == 1)
    return advisor.Say(AdviceText::CALL_SPECIAL_FORCES);
  if (side.strength >= GOOD_RATING && !data.weather && Record<Person>(data, data.context_record3).charm >= 60 && opeu_ready_group_has_idle_unit(game, far_text) && advisor.Random(3) == 1)
    return advisor.Say(AdviceText::PARATROOP_ATTACK);
  return 0;
}

// The side's first linked objective: ours, solidify its defenses; the enemy's, rush in and take it.
uint16_t opeu_advice_tactical_linked_objective(Game& game, const uint8_t* far_text) {
  auto&      data{ Data(game) };
  auto const advisor{ Tactics(game, far_text) };
  auto const objective{ data.objective_links[data.current_side * 21] };
  if (!objective) return 0;
  return advisor.SayFormatted(opeu_objective_side(game, objective) == data.current_side ? AdviceText::SOLIDIFY_DEFENSES : AdviceText::RUSH_OBJECTIVE, objective);
}

// Tactical advice: runs the checks until one gives advice.
void opeu_advice_tactical(Game& game, const uint8_t* far_text) {
  using Check = uint16_t (*)(Game&, const uint8_t*);
  static constexpr Check CHECKS[]{
    opeu_advice_tactical_objective_threat, opeu_advice_tactical_command,    opeu_advice_tactical_reorganize,         opeu_advice_tactical_unit_deployment,
    opeu_advice_tactical_supply_reinforce, opeu_advice_tactical_land_mines, opeu_advice_tactical_air_special_forces, opeu_advice_tactical_linked_objective,
  };
  for (auto const check : CHECKS)
    if (check(game, far_text)) return;
}
