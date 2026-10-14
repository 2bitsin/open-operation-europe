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
#include "types.hpp"
#include "units/unitname.hpp"

#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/specops.hpp"
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

// The special forces menu's missions, by the enumerator special_missions holds for the choice.
constexpr auto SPECIAL_MISSIONS = DispatchRows<SpecialForcesAction, auto (*)(Game&)->void>({
    { SpecialForcesAction::RECON,       opeu_special_recon       },
    { SpecialForcesAction::CUT_LINES,   opeu_special_cut_comm    },
    { SpecialForcesAction::SABOTAGE,    opeu_special_sabotage    },
    { SpecialForcesAction::RAID,        opeu_special_raid        },
    { SpecialForcesAction::ASSASSINATE, opeu_special_assassinate },
});
} // namespace

// Points special_order at the side's order, picks the officer and averages the four abilities the missions test.
void opeu_special_officer(Game& game) {
  auto& data{ Data(game) };
  data.special_order  = Near(data, Record<Side>(data, data.current_side_ptr).order);
  data.special_person = opeu_find_person_template(game, Record<Person>(data, data.context_record1).status, OFFICER_KIND);
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
      opeu_army_title(game, offsetof(DataSegment, special_forces_title));
      opeu_army_position(game, INFO_X, ORDERS_Y);
      opeu_cursor_print(game, std::array<FormatArgument, 2>{ offsetof(DataSegment, special_orders_line), data.special_mission_names[order.kind] });
      opeu_army_position(game, INFO_X, TARGET_Y);
      opeu_cursor_print(game, std::array<FormatArgument, 2>{ offsetof(DataSegment, special_target_line), (opeu_special_unit_target(game) ? opeu_unit_name(game, order.target, name) : order.target) });
      opeu_army_position(game, INFO_X, TIME_Y);
      opeu_cursor_print(game, std::array<FormatArgument, 2>{ offsetof(DataSegment, special_time_line), order.progress });
      if (!opeu_confirm_message(game, offsetof(DataSegment, special_cancel_question), data.context_record2)) break;
      order.kind = Bits(SpecialMission::NONE);
    }
    auto const choice{ opeu_delay_dialog(game, offsetof(DataSegment, special_select_question), MISSION_COUNT, data.context_record2, offsetof(DataSegment, special_menu)) };
    if (choice == MENU_CANCELLED) return;
    Dispatch(SPECIAL_MISSIONS, data.special_missions.at(choice - 1u), game);
  } while (!data.side_delays[data.current_side]);
  opeu_daily_context(game, data.current_side);
}
