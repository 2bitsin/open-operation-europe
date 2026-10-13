// The Special Forces missions. Each mission, run from the Special Forces menu (core/specfrc), has the player pick a
// map cell starting at the viewport, asks the Special Forces officer to confirm an enemy city garrison or a special
// enemy unit there, and lets the officer refuse on random rolls against his abilities (the averages core/specfrc
// keeps). An accepted mission states its duration in hours, base - min(leadership / divisor - delivery level + 5,
// base - 1), plus one when the target cell is not under the side's influence; once the commander approves the
// lieutenant who will lead it, the mission goes into the side's special order and is shown in the daily report. A
// refusal or a stored order ends the side's command phase; abandoning the map pick just returns.
#include "core/specops.hpp"
#include "common/events.hpp"
#include "core/limits.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/recindex.hpp"
#include "core/template.hpp"
#include "core/textout.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "map/cellflag.hpp"
#include "map/mappick.hpp"
#include "map/query.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

using enum SpecialService;

constexpr std::uint16_t NO_TEXT{ 0 }; // no text: the mission has no such step

// The daily report entry of an accepted mission and its presenter, opeu_special_present, as the far address the
// routine table resolves.
constexpr std::uint16_t MISSION_REPORT{ 0x10f };
constexpr FarPointer    MISSION_PRESENTER{ 0x0000, 0x24d5 };

// The message that states a raid's duration.
constexpr std::uint16_t RAID_DURATION_MESSAGE{ 0x7c };

// The texts and figures of one mission.
struct Mission {
  std::uint16_t                title;
  std::uint16_t                prompt;
  std::uint16_t                city_question; // NO_TEXT: the mission does not target cities
  std::uint16_t                unit_question; // NO_TEXT: the mission does not target units
  SpecialMission               city_code;
  SpecialMission               unit_code;
  std::array<std::uint16_t, 4> refusals; // the fourth is NO_TEXT when the officer refuses on three rolls only
  std::uint8_t                 base;     // the hours a mission takes at most (before the influence hour)
  std::uint8_t                 divisor;  // of the officer's leadership
  std::uint16_t                duration; // NO_TEXT: the raid's message from the message table
  std::uint16_t                hour;     // NO_TEXT: the duration takes the hour count only
  std::uint16_t                hours;
  std::uint16_t                lieutenant;
};

// The locals of a mission's planning that services write: offsets in its reserve.
constexpr std::uint16_t NAME_AT{ 0x00 }; // 32 bytes: the target unit's name
constexpr std::uint16_t TEXT_AT{ 0x20 }; // 100 bytes: the officer's question
constexpr std::uint16_t CELL_Y_AT{ 0x84 };
constexpr std::uint16_t CELL_X_AT{ 0x86 };
constexpr std::uint16_t PLAN_LOCALS{ 0x88 };

auto Raid() -> Mission {
  return {
    offsetof(DataSegment, raid_title),
    offsetof(DataSegment, raid_prompt),
    offsetof(DataSegment, raid_city_question),
    offsetof(DataSegment, raid_unit_question),
    SpecialMission::RAID_CITY,
    SpecialMission::RAID_UNIT,
    { offsetof(DataSegment, raid_refusal_1), offsetof(DataSegment, raid_refusal_2), offsetof(DataSegment, raid_refusal_3), offsetof(DataSegment, raid_refusal_4) },
    12,
    19,
    NO_TEXT,
    NO_TEXT,
    NO_TEXT,
    offsetof(DataSegment, raid_lieutenant)
  };
}

auto Sabotage() -> Mission {
  return {
    offsetof(DataSegment, sabotage_title),
    offsetof(DataSegment, sabotage_prompt),
    offsetof(DataSegment, sabotage_city_question),
    NO_TEXT,
    SpecialMission::SABOTAGE,
    SpecialMission::SABOTAGE,
    { offsetof(DataSegment, sabotage_refusal_1), offsetof(DataSegment, sabotage_refusal_2), offsetof(DataSegment, sabotage_refusal_3), offsetof(DataSegment, sabotage_refusal_4) },
    11,
    24,
    offsetof(DataSegment, sabotage_duration),
    NO_TEXT,
    NO_TEXT,
    offsetof(DataSegment, sabotage_lieutenant)
  };
}

auto CutComm() -> Mission {
  return {
    offsetof(DataSegment, cut_comm_title),
    offsetof(DataSegment, cut_comm_prompt),
    offsetof(DataSegment, cut_comm_city_question),
    offsetof(DataSegment, cut_comm_unit_question),
    SpecialMission::CUT_COMM_CITY,
    SpecialMission::CUT_COMM_UNIT,
    { offsetof(DataSegment, cut_comm_refusal_1), offsetof(DataSegment, cut_comm_refusal_2), offsetof(DataSegment, cut_comm_refusal_3), offsetof(DataSegment, cut_comm_refusal_4) },
    10,
    30,
    offsetof(DataSegment, cut_comm_duration),
    offsetof(DataSegment, cut_comm_hour),
    offsetof(DataSegment, cut_comm_hours),
    offsetof(DataSegment, cut_comm_lieutenant)
  };
}

auto Assassinate() -> Mission {
  return {
    offsetof(DataSegment, assassinate_title),
    offsetof(DataSegment, assassinate_prompt),
    NO_TEXT,
    offsetof(DataSegment, assassinate_unit_question),
    SpecialMission::ASSASSINATE,
    SpecialMission::ASSASSINATE,
    { offsetof(DataSegment, assassinate_refusal_1), offsetof(DataSegment, assassinate_refusal_2), offsetof(DataSegment, assassinate_refusal_3), offsetof(DataSegment, assassinate_refusal_4) },
    13,
    24,
    offsetof(DataSegment, assassinate_duration),
    NO_TEXT,
    NO_TEXT,
    offsetof(DataSegment, assassinate_lieutenant)
  };
}

auto Recon() -> Mission {
  return {
    offsetof(DataSegment, recon_title),
    offsetof(DataSegment, recon_prompt),
    offsetof(DataSegment, recon_city_question),
    offsetof(DataSegment, recon_unit_question),
    SpecialMission::RECON_CITY,
    SpecialMission::RECON_UNIT,
    { offsetof(DataSegment, recon_refusal_1), offsetof(DataSegment, recon_refusal_2), offsetof(DataSegment, recon_refusal_3), NO_TEXT },
    4,
    40,
    offsetof(DataSegment, recon_duration),
    offsetof(DataSegment, recon_hour),
    offsetof(DataSegment, recon_hours),
    offsetof(DataSegment, recon_lieutenant)
  };
}

// Which refusal the officer gives, or -1 when he takes the mission. The rolls are drawn in this order and only as far
// as needed.
auto Refusal(Game& game, Mission const& mission) -> int {
  auto const& data{ Data(game) };
  if (RandomBelow(game, 31) + 50 > data.special_leadership) return 0;
  if (RandomBelow(game, 21) + 30 > data.special_charm && RandomBelow(game, 21) + 20 > data.special_experience) return 1;
  if (RandomBelow(game, 21) + 40 > data.special_bravery) return 2;
  if (mission.refusals[3] != NO_TEXT && !RandomBelow(game, 5)) return 3;
  return -1;
}

// Plans one mission; the side's command phase ends unless the player abandons the map pick.
void Plan(Game& game, HostEvent event, Mission const& mission) {
  ServiceLocals const locals{ PLAN_LOCALS };
  auto&               data{ Data(game) };
  auto const          cell_x{ locals.At(CELL_X_AT) };
  auto const          cell_y{ locals.At(CELL_Y_AT) };
  auto const          text{ locals.At(TEXT_AT) };
  auto const          name{ locals.At(NAME_AT) };
  auto&               x{ Record<std::uint8_t>(data, cell_x) };
  auto&               y{ Record<std::uint8_t>(data, cell_y) };
  Call(game, event, ARMY_TITLE, mission.title);
  for (;;) {
    x = static_cast<std::uint8_t>(data.viewport_x);
    y = static_cast<std::uint8_t>(data.viewport_y);
    std::uint16_t objective{ 0 };
    std::uint16_t unit{ 0 };
    bool          on_city{ false };
    for (;;) {
      Call(game, event, CONTROL_MESSAGE, mission.prompt, data.special_person);
      if (!Call(game, event, MAP_PICK_LOCATION, cell_x, cell_y)) return;
      if (mission.city_question != NO_TEXT && (objective = Call(game, event, FIND_OBJECTIVE_AT, x, y)) != 0
          && static_cast<std::uint8_t>(Call(game, event, OBJECTIVE_SIDE, objective)) != data.current_side) {
        Call(game, event, REQUEST_FORMAT, text, mission.city_question, objective);
        if (Call(game, event, CONFIRM_MESSAGE, text, data.special_person)) {
          on_city = true;
          break;
        }
      }
      if (mission.unit_question == NO_TEXT) continue;
      if ((unit = Call(game, event, FIND_UNIT_AT, x, y)) == 0) continue;
      if (static_cast<std::uint8_t>(Call(game, event, UNIT_SIDE, unit)) == data.current_side) continue;
      if (!Call(game, event, UNIT_SPECIAL, unit)) continue;
      Call(game, event, REQUEST_FORMAT, text, mission.unit_question, Call(game, event, UNIT_NAME, unit, name));
      if (!Call(game, event, CONFIRM_MESSAGE, text, data.special_person)) continue;
      break;
    }
    if (auto const refusal{ Refusal(game, mission) }; refusal >= 0) {
      Call(game, event, CONTROL_MESSAGE, mission.refusals[refusal], data.special_person);
      break;
    }
    auto hours{ static_cast<std::uint8_t>(mission.base - opeu_min16(data.special_leadership / mission.divisor - data.delivery_level + 5, mission.base - 1)) };
    hours += static_cast<std::uint8_t>(Call(game, event, CELL_INFLUENCE_SIDE, x, y)) == data.current_side ? 0 : 1;
    if (mission.duration == NO_TEXT)
      Call(game, event, REQUEST_FORMAT, text, Call(game, event, MESSAGE_LOOKUP, RAID_DURATION_MESSAGE), hours);
    else if (mission.hour == NO_TEXT)
      Call(game, event, REQUEST_FORMAT, text, mission.duration, hours);
    else
      Call(game, event, REQUEST_FORMAT, text, mission.duration, hours, hours == 1 ? mission.hour : mission.hours);
    Call(game, event, CONTROL_MESSAGE, text, data.special_person);
    Call(game, event, REQUEST_FORMAT, text, mission.lieutenant, data.special_person);
    if (!Call(game, event, CONFIRM_MESSAGE, text, data.context_record2)) continue;
    auto& order{ Record<Order>(data, data.special_order) };
    order.target   = on_city ? objective : unit;
    order.kind     = Bits(on_city ? mission.city_code : mission.unit_code);
    order.progress = hours;
    Call(game, event, DAILY_SHOW, MISSION_REPORT, data.current_side, MISSION_PRESENTER.offset, MISSION_PRESENTER.segment);
    break;
  }
  data.side_delays[data.current_side] = 1;
}
} // namespace

// The daily report's presenter of an accepted mission.
void opeu_special_present(Game& game, HostEvent event) {
  Call(game, event, PENDING_FINISH, 3);
}

void opeu_special_raid(Game& game, HostEvent event) {
  Plan(game, event, Raid());
}

void opeu_special_sabotage(Game& game, HostEvent event) {
  Plan(game, event, Sabotage());
}

void opeu_special_cut_comm(Game& game, HostEvent event) {
  Plan(game, event, CutComm());
}

void opeu_special_assassinate(Game& game, HostEvent event) {
  Plan(game, event, Assassinate());
}

void opeu_special_recon(Game& game, HostEvent event) {
  Plan(game, event, Recon());
}

uint16_t svc_specops(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r{ 0 };
  switch (static_cast<SpecialService>(id)) {
  case REQUEST_FORMAT:       opeu_request_format(game, { args, count }); break;
  case CONTROL_MESSAGE:      opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case CONFIRM_MESSAGE:      r = opeu_confirm_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case MESSAGE_LOOKUP:       r = opeu_message_lookup(game, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  case ARMY_PRINT:           opeu_cursor_print(game, { args, count }); break;
  case PENDING_FINISH:       opeu_pending_finish(game, (uint8_t)Arg(args, count, 0)); break;
  case DAILY_SHOW:           port_daily_show(game, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case FIND_OBJECTIVE_AT:    r = opeu_find_objective_at(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case DELAY_DIALOG:         r = opeu_delay_dialog(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case ARMY_TITLE:           opeu_army_title(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case MAP_PICK_LOCATION:    r = opeu_map_pick_location(game, svc_mappick, Arg(args, count, 0), Arg(args, count, 1)); break;
  case OBJECTIVE_SIDE:       r = opeu_objective_side(game, Arg(args, count, 0)); break;
  case ARMY_POSITION:        opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case DAILY_CONTEXT:        opeu_daily_context(game, svc_context, (uint8_t)Arg(args, count, 0)); break;
  case FAR_ACTION:           port_call(game, (uint32_t)Arg(args, count, 0) * 16u + Arg(args, count, 1), nullptr, 0); break;
  case FIND_UNIT_AT:         r = opeu_find_unit_at(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case UNIT_SIDE:            r = opeu_unit_side(Arg(args, count, 0)); break;
  case UNIT_SPECIAL:         r = opeu_unit_special(game, Arg(args, count, 0)); break;
  case UNIT_NAME:            r = opeu_unit_name(game, svc_unitname, Arg(args, count, 0), Arg(args, count, 1)); break;
  case CELL_INFLUENCE_SIDE:  r = opeu_cell_influence_side(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case FIND_PERSON_TEMPLATE: r = opeu_find_person_template(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  default:                   port_unhandled("specops", id, count);
  }
  return r;
}

auto port_routine_opeu_special_assassinate(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_special_assassinate(game, svc_specops);
  return 0;
}

auto port_routine_opeu_special_cut_comm(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_special_cut_comm(game, svc_specops);
  return 0;
}

auto port_routine_opeu_special_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_special_present(game, svc_specops);
  return 0;
}

auto port_routine_opeu_special_raid(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_special_raid(game, svc_specops);
  return 0;
}

auto port_routine_opeu_special_recon(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_special_recon(game, svc_specops);
  return 0;
}

auto port_routine_opeu_special_sabotage(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_special_sabotage(game, svc_specops);
  return 0;
}
