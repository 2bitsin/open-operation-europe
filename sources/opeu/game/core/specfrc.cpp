// The Special Forces command (command action 9). The officer is the person template of kind 7 for the commander's
// nation; his abilities for the missions are the averages of the deputy's and the commander's. A pending mission is
// shown with the offer to cancel it; then the player picks a mission to plan (core/specops), until one ends the
// side's command phase.
#include "core/specfrc.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/template.hpp"
#include "core/textout.hpp"
#include "link/link.hpp"
#include "types.hpp"
#include "units/unitname.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/specops.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint8_t  OFFICER_KIND{ 7 };  // the person template kind of the Special Forces officer
constexpr std::uint16_t MISSION_COUNT{ 5 }; // the entries of the mission menu
constexpr std::uint16_t INFO_X{ 0x2d };     // the column of the pending mission's lines
constexpr std::uint16_t ORDERS_Y{ 0xa3 };   // and their rows
constexpr std::uint16_t TARGET_Y{ 0xb5 };
constexpr std::uint16_t TIME_Y{ 0xc7 };
constexpr std::uint16_t NAME_BYTES{ 0x22 }; // the target unit's name, which the unit-name service writes

// The average of the deputy's and the commander's ability.
template <typename Ability>
auto Average(Person const& deputy, Person const& commander, Ability ability) -> std::uint8_t {
  return static_cast<std::uint8_t>((static_cast<int>(deputy.*ability) + commander.*ability) / 2);
}
} // namespace

// Points special_order at the side's order, picks the officer and averages the four abilities the missions test.
void opeu_special_officer(Game& game) {
  auto& data{ Data(game) };
  data.special_order  = Near(data, Record<Side>(data, data.current_side_ptr).order);
  data.special_person = opeu_find_person_template(game, static_cast<uint8_t>(Record<Person>(data, data.context_record1).status), static_cast<uint8_t>(OFFICER_KIND));
  auto const& deputy{ Record<Person>(data, data.context_record2) };
  auto const& commander{ Record<Person>(data, data.context_record1) };
  data.special_leadership = Average(deputy, commander, &Person::leadership);
  data.special_charm      = Average(deputy, commander, &Person::charm);
  data.special_bravery    = Average(deputy, commander, &Person::bravery);
  data.special_experience = Average(deputy, commander, &Person::experience);
}

// Whether the pending mission targets a unit (1) or a city (0); any other code is returned as it is.
uint16_t opeu_special_unit_target(Game& game) {
  auto const& data{ Data(game) };
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

void opeu_turn_action_9(Game& game) {
  ServiceLocals const locals{ NAME_BYTES };
  auto&               data{ Data(game) };
  auto const          name{ locals.At(0) };
  opeu_special_officer(game);
  do {
    auto& order{ Record<Order>(data, data.special_order) };
    if (order.kind != Bits(SpecialMission::NONE)) {
      opeu_army_title(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, special_forces_title));
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(INFO_X), static_cast<int16_t>(ORDERS_Y));
      opeu_cursor_print(game,
                        std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, special_orders_line)), static_cast<std::uint16_t>(data.special_mission_names[order.kind]) });
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(INFO_X), static_cast<int16_t>(TARGET_Y));
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, special_target_line)),
                                                            static_cast<std::uint16_t>(opeu_special_unit_target(game) ? opeu_unit_name(game, order.target, name) : order.target) });
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(INFO_X), static_cast<int16_t>(TIME_Y));
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, special_time_line)), static_cast<std::uint16_t>(order.progress) });
      if (!opeu_confirm_message(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, special_cancel_question), data.context_record2)) break;
      order.kind = Bits(SpecialMission::NONE);
    }
    auto const choice{ opeu_delay_dialog(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, special_select_question), MISSION_COUNT, data.context_record2,
                                         offsetof(DataSegment, special_menu)) };
    if (choice == MENU_CANCELLED) return;
    auto const& mission{ data.special_missions[choice - 1] };
    port_call(game, static_cast<uint32_t>(static_cast<std::uint16_t>(mission.segment)) * 16u + static_cast<std::uint16_t>(mission.offset), nullptr, 0);
  } while (!data.side_delays[data.current_side]);
  opeu_daily_context(game, static_cast<uint8_t>(data.current_side));
}

auto port_routine_opeu_turn_action_9(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_turn_action_9(game);
  return 0;
}
