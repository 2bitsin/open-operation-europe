// The staff's "Enemy Conditions" advice: ten checks run in order and the first that gives advice ends the entry. Each
// writes its advice into the enemy topic's buffer and shows it in the person of the staff officer.
#include "core/advenemy.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/advice.hpp"
#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "core/dialogs.hpp"
#include "core/people.hpp"
#include "core/sideutil.hpp"
#include "glue.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint16_t PLENTY{ 10000 }; // a stock an objective has plenty of
constexpr std::uint8_t  DIRECTIONS{ 8 };
constexpr std::uint8_t  ALERT_OFFICER{ 90 }; // the staff officer's rating that finds the enemy commander

auto Enemy(std::uint8_t* ds, HostEvent event, std::uint8_t const* far_text, std::uint16_t sp) -> Advisor {
  return { ds, far_text, event, AdviceTopic::ENEMY, sp };
}

// Whether an order of kind occupy or attack.
auto IsAssault(std::uint8_t kind) -> bool {
  return kind == Bits(UnitOrder::OCCUPY) || kind == Bits(UnitOrder::ATTACK);
}

// Calls visit(division, unit) for each ready special unit of the enemy's active divisions, in order, until it
// returns true; returns whether it did.
template <typename Visit>
auto ForEnemySpecialUnits(std::uint8_t* ds, Visit&& visit) -> bool {
  auto&      data{ Data(ds) };
  auto const foe{ opeu_opposite_side(data.current_side) };
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    if (!IsActiveGroup(GroupOf(data, foe, g))) continue;
    for (auto& unit : GroupUnits(data, foe, g)) {
      if (!(unit.flags & Bits(UnitFlag::READY)) || !opeu_unit_special(ds, Near(data, unit))) continue;
      if (visit(g, unit)) return true;
    }
  }
  return false;
}
} // namespace

// Side 0 in scenarios 1 and 2: one time in four the enemy is laying a mine field; side 0 in scenario 3 in weather 0
// or 1: one time in four the enemy fleets prepare a bombardment.
uint16_t opeu_advice_enemy_mines_or_bombardment(uint8_t ds[65536], const uint8_t* far_text, HostEvent event, uint16_t sp) {
  auto const& data{ Data(ds) };
  auto const  advisor{ Enemy(ds, event, far_text, sp) };
  auto const  scenario{ CurrentScenario(data) };
  if (data.current_side == 0 && (scenario == Scenario::AFRICA || scenario == Scenario::KURSK) && advisor.Random(4) == 1) return advisor.Say(AdviceText::ENEMY_MINE_FIELD);
  if (data.current_side == 0 && scenario == Scenario::D_DAY && (data.weather == 0 || data.weather == 1) && advisor.Random(4) == 1) return advisor.Say(AdviceText::ENEMY_BOMBARDMENT);
  return 0;
}

// An active enemy division with no ready unit of a fighting category: with chance 1/(7 - g) its commander is unfit
// for battle.
uint16_t opeu_advice_enemy_division_unfit(uint8_t ds[65536], const uint8_t* far_text, HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const advisor{ Enemy(ds, event, far_text, sp) };
  auto const foe{ opeu_opposite_side(data.current_side) };
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    if (!IsActiveGroup(GroupOf(data, foe, g))) continue;
    auto const   units{ GroupUnits(data, foe, g) };
    std::uint8_t t{ 0 };
    for (; t < UNITS_PER_GROUP; ++t)
      if ((units[t].flags & Bits(UnitFlag::READY)) && opeu_category_0_to_5(t)) break;
    if (t == UNITS_PER_GROUP && advisor.Random(static_cast<std::uint16_t>(7 - g)) == 1) return advisor.SayFormatted(advisor.Scratch(0x6e), AdviceText::ENEMY_COMMANDER_UNFIT, g + 1);
  }
  return 0;
}

// The enemy's air strike, airborne drop or airlift under way: each one time in four.
uint16_t opeu_advice_enemy_air_operations(uint8_t ds[65536], const uint8_t* far_text, HostEvent event, uint16_t sp) {
  auto&       data{ Data(ds) };
  auto const  advisor{ Enemy(ds, event, far_text, sp) };
  auto const& enemy{ data.sides[opeu_opposite_side(data.current_side)] };
  if (enemy.strike.planes != 0 && advisor.Random(4) == 1) return advisor.Say(AdviceText::ENEMY_AIR_RAIDS);
  if (enemy.drop.transports != 0 && advisor.Random(4) == 1) return advisor.Say(AdviceText::ENEMY_PARATROOPS);
  if (enemy.lift.planes != 0 && advisor.Random(4) == 1) return advisor.Say(AdviceText::ENEMY_CARGO_PLANES);
  return 0;
}

// When no enemy special unit with an occupy or attack order stands next to a cell of ours: one time in four the enemy
// has no plans to attack. Otherwise the first pass looks for such a unit whose order has started (its attack comes
// shortly), the second for a resting one; each with chance 1/(7 - g).
uint16_t opeu_advice_enemy_attack_plans(uint8_t ds[65536], const uint8_t* far_text, HostEvent event, uint16_t sp) {
  auto&        data{ Data(ds) };
  auto const   advisor{ Enemy(ds, event, far_text, sp) };
  std::uint8_t threats{ 0 };
  ForEnemySpecialUnits(ds, [&](std::uint8_t, Unit const& unit) {
    if (IsAssault(unit.orders[0].kind))
      for (std::uint8_t d{ 0 }; d < DIRECTIONS; ++d) {
        std::uint8_t x{ unit.x };
        std::uint8_t y{ unit.y };
        if (opeu_grid_step(&x, &y, d) && opeu_cell_side(ds, x, y) == data.current_side) {
          ++threats;
          break;
        }
      }
    return false;
  });
  if (threats == 0 && advisor.Random(4) == 1) return advisor.Say(AdviceText::ENEMY_NO_ATTACK_PLANS);
  auto const name{ advisor.Scratch(0x32) };
  auto const text{ advisor.Scratch(0x96) };
  for (std::uint8_t pass{ 0 }; pass < 2; ++pass) {
    std::uint16_t given{ 0 };
    ForEnemySpecialUnits(ds, [&](std::uint8_t g, Unit& unit) {
      auto const& order{ unit.orders[0] };
      auto const  chance{ static_cast<std::uint16_t>(7 - g) };
      if (IsAssault(order.kind)) {
        if (pass == 0 && order.progress == 1 && advisor.Random(chance) == 1) given = advisor.SayFormatted(text, AdviceText::ENEMY_ATTACK_SOON, advisor.UnitName(Near(data, unit), name));
      } else if (order.kind == Bits(UnitOrder::REST)) {
        if (pass == 1 && advisor.Random(chance) == 1) given = advisor.SayFormatted(text, AdviceText::ENEMY_RESTING, advisor.UnitName(Near(data, unit), name));
      }
      return given != 0;
    });
    if (given) return given;
  }
  return 0;
}

// The enemy's first linked objective: one time in four, held by us the enemy plots to take it, held by the enemy it
// strengthens its defenses there.
uint16_t opeu_advice_enemy_objective_intent(uint8_t ds[65536], const uint8_t* far_text, HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const advisor{ Enemy(ds, event, far_text, sp) };
  auto const foe{ opeu_opposite_side(data.current_side) };
  auto const target{ data.objective_links[foe * 21] };
  if (target != 0) {
    if (opeu_objective_side(ds, target) == data.current_side && advisor.Random(4) == 1) return advisor.SayFormatted(advisor.Scratch(0x68), AdviceText::ENEMY_PLOTS_TO_TAKE, target);
    if (opeu_objective_side(ds, target) == foe && advisor.Random(4) == 1) return advisor.SayFormatted(advisor.Scratch(0x68), AdviceText::ENEMY_STRENGTHENS, target);
  }
  return 0;
}

// The enemy side has a standing order: one time in four its recon teams are active. Otherwise a ready unit of ours
// that the enemy has reconnoitred: one time in four the enemy knows our plans.
uint16_t opeu_advice_enemy_recon_activity(uint8_t ds[65536], const uint8_t* far_text, HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const advisor{ Enemy(ds, event, far_text, sp) };
  if (data.sides[opeu_opposite_side(data.current_side)].order.kind != Bits(UnitOrder::NONE) && advisor.Random(4) == 1) return advisor.Say(AdviceText::ENEMY_RECON_ACTIVE);
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    if (!(GroupOf(data, data.current_side, g).flags & Bits(GroupFlag::READY))) continue;
    for (auto const& unit : GroupUnits(data, data.current_side, g))
      if ((unit.flags & Bits(UnitFlag::READY)) && (unit.flags & Bits(UnitFlag::SPOTTED)) && advisor.Random(4) == 1) return advisor.Say(AdviceText::ENEMY_KNOWS_PLANS);
  }
  return 0;
}

// An objective in use not ours with plenty of all three stocks: one time in four we hear of its materials.
uint16_t opeu_advice_enemy_plentiful_materials(uint8_t ds[65536], const uint8_t* far_text, HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const advisor{ Enemy(ds, event, far_text, sp) };
  for (auto const& objective : data.objectives) {
    auto const near{ Near(data, objective) };
    if (objective.active && opeu_objective_side(ds, near) != data.current_side && objective.resources[0] >= PLENTY && objective.resources[1] >= PLENTY && objective.resources[2] >= PLENTY
        && advisor.Random(4) == 1)
      return advisor.SayFormatted(advisor.Scratch(0x68), AdviceText::PLENTY_OF_MATERIALS, near);
  }
  return 0;
}

// Enemy special units: the first pass looks for a slow one (average11 >= 70, average9 <= 20), the second for an
// active one (average11 <= 5, average9 >= 90); each with chance 1/(7 - g).
uint16_t opeu_advice_enemy_unit_activity(uint8_t ds[65536], const uint8_t* far_text, HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const advisor{ Enemy(ds, event, far_text, sp) };
  auto const name{ advisor.Scratch(0x2c) };
  auto const text{ advisor.Scratch(0x90) };
  for (std::uint8_t pass{ 0 }; pass < 2; ++pass) {
    std::uint16_t given{ 0 };
    ForEnemySpecialUnits(ds, [&](std::uint8_t g, Unit& unit) {
      auto const near{ Near(data, unit) };
      auto const chance{ static_cast<std::uint16_t>(7 - g) };
      if (pass == 0 && opeu_unit_average11(ds, near) >= 70 && opeu_unit_average9(ds, near) <= 20 && advisor.Random(chance) == 1)
        given = advisor.SayFormatted(text, AdviceText::ENEMY_SLOWING, advisor.UnitName(near, name));
      else if (pass == 1 && opeu_unit_average11(ds, near) <= 5 && opeu_unit_average9(ds, near) >= 90 && advisor.Random(chance) == 1)
        given = advisor.SayFormatted(text, AdviceText::ENEMY_ACTIVE, advisor.UnitName(near, name));
      return given != 0;
    });
    if (given) return given;
  }
  return 0;
}

// A staff officer with a leadership of 90 or more, one time in 24: names the enemy army's commander and the first
// enemy division that is selected.
uint16_t opeu_advice_enemy_commander_location(uint8_t ds[65536], const uint8_t* far_text, HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const advisor{ Enemy(ds, event, far_text, sp) };
  if (Record<Person>(data, data.context_record2).leadership >= ALERT_OFFICER && advisor.Random(24) == 1) {
    auto const foe{ opeu_opposite_side(data.current_side) };
    auto const leader{ opeu_find_side_record(ds, foe, 1) };
    if (leader != 0) {
      std::uint8_t g{ 0 };
      for (; g < GROUPS_PER_SIDE; ++g)
        if (GroupOf(data, foe, g).flags & Bits(GroupFlag::SELECTED)) break;
      return advisor.SayFormatted(advisor.Scratch(0x6a), AdviceText::ENEMY_COMMANDER_FOUND, leader, g + 1);
    }
  }
  return 0;
}

// The fallback: we need recon teams.
uint16_t opeu_advice_enemy_need_recon(uint8_t ds[65536], const uint8_t* far_text, HostEvent event, uint16_t sp) {
  return Enemy(ds, event, far_text, sp).Say(AdviceText::NEED_RECON);
}

// Enemy Conditions: runs the checks until one gives advice.
void opeu_advice_enemy_conditions(uint8_t ds[65536], const uint8_t* far_text, HostEvent event, uint16_t sp) {
  using Check = uint16_t (*)(uint8_t*, const uint8_t*, HostEvent, uint16_t);
  static constexpr Check CHECKS[]{
    opeu_advice_enemy_mines_or_bombardment, opeu_advice_enemy_division_unfit,      opeu_advice_enemy_air_operations, opeu_advice_enemy_attack_plans,       opeu_advice_enemy_objective_intent,
    opeu_advice_enemy_recon_activity,       opeu_advice_enemy_plentiful_materials, opeu_advice_enemy_unit_activity,  opeu_advice_enemy_commander_location, opeu_advice_enemy_need_recon,
  };
  auto const nested{ static_cast<std::uint16_t>(sp - 4) };
  for (auto const check : CHECKS)
    if (check(ds, far_text, event, nested)) return;
}

uint16_t svc_advenemy(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_advice("advenemy", ds, id, args, count);
}

// The services every advice check calls through its event.
uint16_t svc_advice(const char* module, uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case ADVICE_FORMAT:  c2_request_format(ds, args, count); break;
  case ADVICE_MESSAGE: opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  default:             port_unhandled(module, id, count);
  }
  return 0;
}

auto port_routine_opeu_advice_enemy_conditions(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  WithStack(PORT_FRAME, [&](std::uint16_t sp) { opeu_advice_enemy_conditions(ds, AdviceTexts(), svc_advenemy, sp); });
  return 0;
}
