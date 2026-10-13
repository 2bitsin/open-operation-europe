// The Air Force command: chooses the air-force officer and his ratings, then lists the pending air strike, airborne
// drop and airlift, asks which mission to plan and runs it, until the side's turn is used up.
#include "core/airforce.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/template.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/labels.hpp"

#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/airbomb.hpp"
#include "core/airdrop.hpp"
#include "core/airlift.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {

constexpr std::uint8_t  OFFICER_KIND{ 6 };      // the person template of the air-force officer
constexpr std::uint16_t LOCATION_BYTES{ 0x28 }; // the location name buffer of the command, which a format service writes
constexpr std::uint16_t TABLE_X{ 0x2d };
constexpr std::uint16_t TABLE_Y{ 0xb5 };
constexpr std::uint16_t ROW_HEIGHT{ 0x12 };

auto Average(std::uint8_t a, std::uint8_t b) -> std::uint8_t {
  return static_cast<std::uint8_t>((a + b) / 2);
}

// The air force menu's missions, by the enumerator air_missions holds for the choice.
constexpr auto AIR_MISSIONS = DispatchRows<AirMission, auto (*)(Game&)->void>({
    { AirMission::STRIKE,       opeu_air_bomb_command },
    { AirMission::PARATROOPERS, opeu_air_drop_command },
    { AirMission::CARGO_DROP,   opeu_air_lift_command },
});
} // namespace

// Picks the air-force officer for the commander's nations; his ratings are the averages of the commander's and the
// chief of staff's leadership, charm and experience.
void opeu_air_officer(Game& game) {
  auto&       data{ Data(game) };
  auto const& commander{ Record<Person>(data, data.context_record1) };
  auto const& staff{ Record<Person>(data, data.context_record3) };
  data.air_person     = opeu_find_person_template(game, commander.status, OFFICER_KIND);
  data.air_leadership = Average(staff.leadership, commander.leadership);
  data.air_charm      = Average(staff.charm, commander.charm);
  data.air_experience = Average(staff.experience, commander.experience);
}

// The mission table header.
void opeu_air_header(Game& game) {
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(0x38), static_cast<short>(0xa3));
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(Near(Data(game), Data(game).air_header_format)) });
}

// The Air Force command loop.
void opeu_turn_action_8(Game& game) {
  ServiceLocals const locals{ LOCATION_BYTES };
  auto&               data{ Data(game) };
  auto const          buffer{ locals.At(0) };
  opeu_air_officer(game);
  do {
    auto const&   side{ Record<Side>(data, data.current_side_ptr) };
    std::uint16_t row{ 0 };
    opeu_army_title(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.air_title));
    if (side.strike.planes) {
      opeu_air_header(game);
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(TABLE_X), static_cast<short>(TABLE_Y));
      opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(Near(data, data.air_strike_format)),
                                                            static_cast<std::uint16_t>(opeu_location_name(game, side.strike.x, side.strike.y, buffer)), static_cast<std::uint16_t>(side.strike.planes),
                                                            static_cast<std::uint16_t>(side.strike.hours) });
      row = 1;
    }
    if (side.drop.transports) {
      opeu_air_header(game);
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(TABLE_X), static_cast<short>(row * ROW_HEIGHT + TABLE_Y));
      opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(Near(data, data.air_drop_format)),
                                                            static_cast<std::uint16_t>(opeu_location_name(game, side.drop.x, side.drop.y, buffer)), static_cast<std::uint16_t>(side.drop.transports),
                                                            static_cast<std::uint16_t>(side.drop.hours) });
      ++row;
    }
    if (side.lift.planes) {
      opeu_air_header(game);
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(TABLE_X), static_cast<short>(row * ROW_HEIGHT + TABLE_Y));
      opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(Near(data, data.air_lift_format)),
                                                            static_cast<std::uint16_t>(opeu_location_name(game, side.lift.x, side.lift.y, buffer)), static_cast<std::uint16_t>(side.lift.planes),
                                                            static_cast<std::uint16_t>(side.lift.hours) });
    }
    auto const choice{ opeu_delay_dialog(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.air_mission_question), 3, data.context_record2,
                                         Near(data, data.air_mission_menu)) };
    if (choice == MENU_CANCELLED) break;
    Dispatch(AIR_MISSIONS, data.air_missions.at(choice - 1u), game);
    opeu_daily_context(game, static_cast<uint8_t>(data.current_side));
  } while (!data.side_delays[data.current_side]);
}
