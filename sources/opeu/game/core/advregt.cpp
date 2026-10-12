// The staff's "Regiment Status" advice: each check walks the current side's ready divisions and their regiments and
// reports the first match into the regiments topic's buffer, in the person of the staff officer; the entry runs the
// checks in order until one reports.
#include "types.hpp"
#include "core/advregt.hpp"
#include "common/events.hpp"

#include "common/advice.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "core/advenemy.hpp"
#include "core/people.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "units/capacity.hpp"
#include "units/ratio.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint16_t SHORT_SUPPLY{ 10 }; // percent of capacity

auto Regiments(std::uint8_t* ds, HostEvent event, std::uint8_t const* far_text,
               std::uint16_t sp) -> Advisor {
  return { ds, far_text, event, AdviceTopic::REGIMENTS, sp };
}

// Calls visit(division, slot, unit) for each regiment of the side's divisions that are ready (and, with
// `active_only`, not in reserve), until it returns true; returns whether it did.
template <typename Visit>
auto ForRegiments(std::uint8_t* ds, bool active_only, Visit&& visit) -> bool {
  auto&      data{ Data(ds) };
  auto const side{ data.current_side };
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    auto const& group{ GroupOf(data, side, g) };
    if (!(group.flags & Bits(GroupFlag::READY)) ||
        (active_only && (group.flags & Bits(GroupFlag::RESERVE))))
      continue;
    auto const units{ GroupUnits(data, side, g) };
    for (std::uint8_t k{ 0 }; k < UNITS_PER_GROUP; ++k)
      if (visit(g, k, units[k]))
        return true;
  }
  return false;
}

// Whether the regiment is ready and can act.
auto IsEligible(std::uint8_t* ds, Unit const& unit) -> bool {
  return (unit.flags & Bits(UnitFlag::READY)) &&
         opeu_unit_selection_eligible(ds, Near(Data(ds), unit));
}

// Gives `text` about the regiment, formatted with its name, through the frame buffers at `scratch` and `name`.
auto SayAboutUnit(Advisor const& advisor, std::uint16_t scratch,
                  std::uint16_t name, AdviceText text, Unit const& unit)
    -> std::uint16_t {
  return advisor.SayFormatted(
      scratch, text, advisor.UnitName(Near(Data(advisor.ds), unit), name));
}
} // namespace

// A regiment about to finish its order (progress 1), with chance 1/(7 - g): an occupy or attack order, the regiment
// will be ready for battle shortly; a mine or bridge order of an engineer (category 6), or a supply or repair order
// of a supply regiment (category 7), its operations finish shortly.
uint16_t opeu_advice_regiment_order_finishing(uint8_t        ds[65536],
                                              const uint8_t* far_text,
                                              HostEvent event, uint16_t sp) {
  auto const    advisor{ Regiments(ds, event, far_text, sp) };
  std::uint16_t given{ 0 };
  ForRegiments(ds, false, [&](std::uint8_t g, std::uint8_t k, Unit& unit) {
    if (!IsEligible(ds, unit))
      return false;
    auto const& order{ unit.orders[0] };
    auto const  chance{ static_cast<std::uint16_t>(7 - g) };
    auto const  kind{ order.kind };
    AdviceText  text{};
    if (kind == Bits(UnitOrder::OCCUPY) || kind == Bits(UnitOrder::ATTACK)) {
      if (order.progress != 1 || advisor.Random(chance) != 1)
        return false;
      text = AdviceText::UNIT_READY_SOON;
    } else if (kind >= Bits(UnitOrder::LAY_MINES) && kind <= 7) {
      if (!opeu_category_6(k) || order.progress != 1 ||
          advisor.Random(chance) != 1)
        return false;
      text = AdviceText::OPERATION_FINISHING;
    } else if (kind >= Bits(UnitOrder::RESUPPLY) && kind <= 11) {
      if (!opeu_category_7(k) || order.progress != 1 ||
          advisor.Random(chance) != 1)
        return false;
      text = AdviceText::OPERATION_FINISHING;
    } else {
      return false;
    }
    given = SayAboutUnit(advisor, advisor.Scratch(0x8c), advisor.Scratch(0x28),
                         text, unit);
    return true;
  });
  return given;
}

// A regiment of an active division holding at most 10 percent of its capacity of a supply, with chance 1/(7 - g)
// for each supply in turn: the regiment is short of it.
uint16_t opeu_advice_regiment_supply_shortage(uint8_t        ds[65536],
                                              const uint8_t* far_text,
                                              HostEvent event, uint16_t sp) {
  auto&         data{ Data(ds) };
  auto const    advisor{ Regiments(ds, event, far_text, sp) };
  std::uint16_t given{ 0 };
  ForRegiments(ds, true, [&](std::uint8_t g, std::uint8_t, Unit& unit) {
    if (!IsEligible(ds, unit))
      return false;
    auto const    near{ Near(data, unit) };
    std::uint16_t capacity[3]{};
    opeu_unit_capacities(ds, near, &capacity[0], &capacity[1], &capacity[2]);
    for (std::uint8_t supply{ 0 }; supply < 3; ++supply)
      if (opeu_resource_percent(unit.resources[supply], capacity[supply]) <=
              SHORT_SUPPLY &&
          advisor.Random(static_cast<std::uint16_t>(7 - g)) == 1) {
        given = advisor.SayFormatted(
            advisor.Scratch(0x96), AdviceText::UNIT_SHORT_OF,
            advisor.UnitName(near, advisor.Scratch(0x32)),
            data.supply_names[supply]);
        return true;
      }
    return false;
  });
  return given;
}

// Division commanders whose health is below a tenth of byte_17, one time in four: the first pass those in poor health
// (seriously wounded), the second the others (fatigued).
uint16_t opeu_advice_division_commander_condition(uint8_t        ds[65536],
                                                  const uint8_t* far_text,
                                                  HostEvent      event,
                                                  uint16_t       sp) {
  auto&      data{ Data(ds) };
  auto const advisor{ Regiments(ds, event, far_text, sp) };
  for (std::uint8_t pass{ 0 }; pass < 2; ++pass)
    for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
      auto const near{ opeu_find_group_record(ds, data.current_side, g) };
      if (!near)
        continue;
      auto const& person{ Record<Person>(data, near) };
      bool const  poor_health{ (person.role & Bits(PersonRole::POOR_HEALTH)) !=
                               0 };
      if (poor_health == (pass == 0) && person.byte_17 / 10 > person.health &&
          advisor.Random(4) == 1)
        return advisor.SayFormatted(advisor.Scratch(0x6a),
                                    pass == 0 ? AdviceText::COMMANDER_WOUNDED
                                              : AdviceText::COMMANDER_FATIGUED,
                                    g + 1, near);
    }
  return 0;
}

// Regiments of active divisions, with chance 1/(7 - g): the first pass looks for one that can act with more than half
// its arms damaged, the second for a ready one that cannot act (its communications are out).
uint16_t opeu_advice_regiment_damage_or_comms(uint8_t        ds[65536],
                                              const uint8_t* far_text,
                                              HostEvent event, uint16_t sp) {
  auto&         data{ Data(ds) };
  auto const    advisor{ Regiments(ds, event, far_text, sp) };
  std::uint16_t given{ 0 };
  for (std::uint8_t pass{ 0 }; pass < 2 && !given; ++pass)
    ForRegiments(ds, true, [&](std::uint8_t g, std::uint8_t, Unit& unit) {
      auto const chance{ static_cast<std::uint16_t>(7 - g) };
      AdviceText text{};
      if (pass == 0 && IsEligible(ds, unit)) {
        std::uint16_t damaged{ 0 };
        std::uint16_t quantity{ 0 };
        for (auto const& element : Elements(data, unit.first)) {
          quantity += element.quantity;
          damaged += element.damage;
        }
        if (static_cast<std::uint16_t>(damaged * 2u) <= quantity ||
            advisor.Random(chance) != 1)
          return false;
        text = AdviceText::DAMAGED_ARMS;
      } else if (pass == 1 && (unit.flags & Bits(UnitFlag::READY)) &&
                 !opeu_unit_selection_eligible(ds, Near(data, unit)) &&
                 advisor.Random(chance) == 1) {
        text = AdviceText::COMMUNICATIONS_OUT;
      } else {
        return false;
      }
      given = SayAboutUnit(advisor, advisor.Scratch(0x94),
                           advisor.Scratch(0x30), text, unit);
      return true;
    });
  return given;
}

// Regiments of active divisions that can act, in four passes, each with chance 1/(7 - g): exhausted (average11 above
// 80), rested (average11 zero), high morale (average9 above 80), morale dropping (average9 below 10).
uint16_t opeu_advice_regiment_fatigue_morale(uint8_t        ds[65536],
                                             const uint8_t* far_text,
                                             HostEvent event, uint16_t sp) {
  auto&         data{ Data(ds) };
  auto const    advisor{ Regiments(ds, event, far_text, sp) };
  std::uint16_t given{ 0 };
  for (std::uint8_t pass{ 0 }; pass < 4 && !given; ++pass)
    ForRegiments(ds, true, [&](std::uint8_t g, std::uint8_t, Unit& unit) {
      if (!IsEligible(ds, unit))
        return false;
      auto const near{ Near(data, unit) };
      bool       matches{};
      AdviceText text{};
      switch (pass) {
        case 0:
          matches = opeu_unit_average11(ds, near) > 80;
          text    = AdviceText::UNIT_EXHAUSTED;
          break;
        case 1:
          matches = !opeu_unit_average11(ds, near);
          text    = AdviceText::UNIT_RESTED;
          break;
        case 2:
          matches = opeu_unit_average9(ds, near) > 80;
          text    = AdviceText::MORALE_HIGH;
          break;
        default:
          matches = opeu_unit_average9(ds, near) < 10;
          text    = AdviceText::MORALE_DROPPING;
          break;
      }
      if (!matches || advisor.Random(static_cast<std::uint16_t>(7 - g)) != 1)
        return false;
      given = SayAboutUnit(advisor, advisor.Scratch(0x8e),
                           advisor.Scratch(0x2a), text, unit);
      return true;
    });
  return given;
}

// The fallback: nothing to report.
uint16_t opeu_advice_regiment_nothing(uint8_t        ds[65536],
                                      const uint8_t* far_text, HostEvent event,
                                      uint16_t sp) {
  return Regiments(ds, event, far_text, sp).Say(AdviceText::NOTHING_TO_REPORT);
}

// Regiment Status: runs the checks until one reports.
uint16_t opeu_advice_regiment_status(uint8_t ds[65536], const uint8_t* far_text,
                                     HostEvent event, uint16_t sp) {
  using Check = uint16_t (*)(uint8_t*, const uint8_t*, HostEvent, uint16_t);
  static constexpr Check CHECKS[]{
    opeu_advice_regiment_order_finishing,
    opeu_advice_regiment_supply_shortage,
    opeu_advice_division_commander_condition,
    opeu_advice_regiment_damage_or_comms,
    opeu_advice_regiment_fatigue_morale,
    opeu_advice_regiment_nothing,
  };
  auto const nested{ static_cast<std::uint16_t>(sp - 4) };
  for (auto const check : CHECKS)
    if (auto const result{ check(ds, far_text, event, nested) })
      return result;
  return 0;
}

uint16_t svc_advregt(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  return svc_advice("advregt", ds, id, args, count);
}

auto port_routine_opeu_advice_regiment_status(std::uint8_t* ds,
                                              std::uint16_t const*, unsigned)
    -> std::uint32_t {
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_advice_regiment_status(ds, AdviceTexts(), svc_advregt, sp);
  });
}
