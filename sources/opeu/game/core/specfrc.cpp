// The Special Forces command (command action 9). The officer is the person template of kind 7 for the commander's
// nation; his abilities for the missions are the averages of the deputy's and the commander's. A pending mission is
// shown with the offer to cancel it; then the player picks a mission to plan (core/specops), until one ends the
// side's command phase.
#include "core/specfrc.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/specops.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

using enum SpecialService;

constexpr std::uint8_t  OFFICER_KIND{ 7 };   // the person template kind of the Special Forces officer
constexpr std::uint16_t MISSION_COUNT{ 5 };  // the entries of the mission menu
constexpr std::uint16_t CANCELLED{ 0xffff }; // the mission menu was left
constexpr std::uint16_t INFO_X{ 0x2d };      // the column of the pending mission's lines
constexpr std::uint16_t ORDERS_Y{ 0xa3 };    // and their rows
constexpr std::uint16_t TARGET_Y{ 0xb5 };
constexpr std::uint16_t TIME_Y{ 0xc7 };
constexpr std::uint16_t NAME_BYTES{ 0x22 };    // the target unit's name, which the unit-name service writes
constexpr std::uint16_t OFFICER_STACK{ 0x2c }; // the stack depth the officer and target routines enter at

// The average of the deputy's and the commander's ability.
template <typename Ability>
auto Average(Person const& deputy, Person const& commander, Ability ability) -> std::uint8_t {
  return static_cast<std::uint8_t>((static_cast<int>(deputy.*ability) + commander.*ability) / 2);
}
} // namespace

// Points special_order at the side's order, picks the officer and averages the four abilities the missions test.
void opeu_special_officer(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  (void)sp;
  auto& data{ Data(ds) };
  data.special_order  = Near(data, Record<Side>(data, data.current_side_ptr).order);
  data.special_person = Call(ds, event, FIND_PERSON_TEMPLATE, Record<Person>(data, data.context_record1).status, OFFICER_KIND);
  auto const& deputy{ Record<Person>(data, data.context_record2) };
  auto const& commander{ Record<Person>(data, data.context_record1) };
  data.special_leadership = Average(deputy, commander, &Person::leadership);
  data.special_charm      = Average(deputy, commander, &Person::charm);
  data.special_bravery    = Average(deputy, commander, &Person::bravery);
  data.special_experience = Average(deputy, commander, &Person::experience);
}

// Whether the pending mission targets a unit (1) or a city (0); any other code is returned as it is.
uint16_t opeu_special_unit_target(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  (void)event, (void)sp;
  auto const& data{ Data(ds) };
  auto const  mission{ Record<Order>(data, data.special_order).kind };
  switch (mission) {
  case Bits(SpecialMission::RAID_UNIT):
  case Bits(SpecialMission::CUT_COMM_UNIT):
  case Bits(SpecialMission::ASSASSINATE):
  case Bits(SpecialMission::RECON_UNIT):    return 1;
  case Bits(SpecialMission::RAID_CITY):
  case Bits(SpecialMission::SABOTAGE):
  case Bits(SpecialMission::CUT_COMM_CITY):
  case Bits(SpecialMission::RECON_CITY):    return 0;
  }
  return mission;
}

void opeu_turn_action_9(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  ServiceLocals const locals{ NAME_BYTES };
  auto&               data{ Data(ds) };
  auto const          name{ locals.At(0) };
  auto const          inner{ static_cast<std::uint16_t>(sp - OFFICER_STACK) };
  opeu_special_officer(ds, event, inner);
  do {
    auto& order{ Record<Order>(data, data.special_order) };
    if (order.kind != Bits(SpecialMission::NONE)) {
      Call(ds, event, ARMY_TITLE, offsetof(DataSegment, special_forces_title));
      Call(ds, event, ARMY_POSITION, INFO_X, ORDERS_Y);
      Call(ds, event, ARMY_PRINT, offsetof(DataSegment, special_orders_line), data.special_mission_names[order.kind]);
      Call(ds, event, ARMY_POSITION, INFO_X, TARGET_Y);
      Call(ds, event, ARMY_PRINT, offsetof(DataSegment, special_target_line), opeu_special_unit_target(ds, event, inner) ? Call(ds, event, UNIT_NAME, order.target, name) : order.target);
      Call(ds, event, ARMY_POSITION, INFO_X, TIME_Y);
      Call(ds, event, ARMY_PRINT, offsetof(DataSegment, special_time_line), order.progress);
      if (!Call(ds, event, CONFIRM_MESSAGE, offsetof(DataSegment, special_cancel_question), data.context_record2)) break;
      order.kind = Bits(SpecialMission::NONE);
    }
    auto const choice{ Call(ds, event, DELAY_DIALOG, offsetof(DataSegment, special_select_question), MISSION_COUNT, data.context_record2, offsetof(DataSegment, special_menu)) };
    if (choice == CANCELLED) return;
    auto const& mission{ data.special_missions[choice - 1] };
    Call(ds, event, FAR_ACTION, mission.segment, mission.offset);
  } while (!data.side_delays[data.current_side]);
  Call(ds, event, DAILY_CONTEXT, data.current_side);
}

uint16_t svc_specfrc(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_specops(ds, id, args, count);
}

auto port_routine_opeu_turn_action_9(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  return WithStack(PORT_FRAME, [ds](std::uint16_t sp) {
    opeu_turn_action_9(ds, svc_specfrc, sp);
    return std::uint32_t{ 0 };
  });
}
