// The Bombing Attack command: the player picks a map site and a mission type (carpet bombing, attack sortie,
// strafing run); the air officer may object (low fuel, too few bombers, poor crews, or by chance), otherwise the
// sortie is announced and, once confirmed, stored as the current side's air strike. Also svc_air, the services of
// all the air force's commands.
//
// Routine frames live on the DS stack: sp is the entry SP, a local whose address reaches a callee sits at
// Local(sp, 2 + n).
#include "core/airbomb.hpp"
#include "common/events.hpp"
#include "core/airhelp.hpp"
#include "core/limits.hpp"
#include "map/grid.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "common/air-service.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/textout.hpp"
#include "glue.hpp"
#include "io/keyboard.hpp"
#include "io/listdlg.hpp"
#include "map/mappick.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>
#include <optional>

using namespace opeu::game;

namespace {

using enum AirService;

// Message ids (message_lookup).
constexpr std::uint16_t LOW_FUEL{ 0x50 };
constexpr std::uint16_t TOO_FEW_BOMBERS{ 0x51 };
constexpr std::uint16_t POOR_CREWS{ 0x52 };
constexpr std::uint16_t ANNOUNCE{ 0x5f };     // hours, "hour"/"hours", planes, "plane"/"planes", mission name
constexpr std::uint16_t CANCEL_ORDER{ 0x60 }; // a strike is pending: cancel it?
constexpr std::uint16_t NO_BOMBERS{ 0x61 };
constexpr std::uint16_t CONFIRM_MISSION{ 0x62 };
constexpr std::uint16_t CHOOSE_TYPE{ 0x68 };

constexpr std::uint16_t UNDERSTOOD{ offsetof(DataSegment, air_understood) };
constexpr std::uint16_t HOUR_TEXT{ offsetof(DataSegment, air_hour) };
constexpr std::uint16_t HOURS_TEXT{ offsetof(DataSegment, air_hours) };
constexpr std::uint16_t PLANE_TEXT{ offsetof(DataSegment, air_plane) };
constexpr std::uint16_t PLANES_TEXT{ offsetof(DataSegment, air_planes) };
constexpr std::uint16_t BOMB_TITLE{ offsetof(DataSegment, bombing_title) };
constexpr std::uint16_t SITE_PROMPT{ offsetof(DataSegment, bombing_site_prompt) };
constexpr std::uint16_t TYPE_MENU{ offsetof(DataSegment, bombing_menu) };

constexpr std::uint16_t CANCELED{ 0xffff };
constexpr std::uint8_t  CITY_TERRAIN{ 1 };
constexpr std::uint16_t TEXT_LOCAL{ 0x66 }; // the announcement's text in opeu_air_bomb_order's frame
constexpr std::uint16_t SITE_X_LOCAL{ 6 };  // the site picked, in opeu_air_bomb_command's frame
constexpr std::uint16_t SITE_Y_LOCAL{ 8 };
constexpr std::uint16_t ORDER_DEPTH{ 0x6e + 10 + 4 }; // opeu_air_bomb_order's frame, saved registers, far return
constexpr std::uint16_t COMMAND_DEPTH{ 0x0a + 4 };    // opeu_air_bomb_command's frame, far return

// A test that objects when `value` is below `limit` and, if the test has a roll, when a random number below 10 plus
// `roll` beats the air force's skill too.
struct Shortage {
  std::uint16_t limit;
  std::uint8_t  roll; // 0: no roll
  std::uint16_t message;
};

// The objections to one mission type against one kind of target, tried in order: fuel, bombers, crews (a random
// number below 20 plus crew_roll beats the skill while the morale is below morale_limit), chance (1 in `chance`).
struct Objections {
  Shortage      fuel;
  Shortage      bombers;
  std::uint8_t  crew_roll;
  std::uint8_t  morale_limit;
  std::uint16_t chance;
  std::uint16_t chance_message;
};

// By mission type, then open ground (0) or a city (1).
// clang-format off
constexpr Objections OBJECTIONS[3][2]{
  { { { 51,  0, LOW_FUEL }, { 40,  0, TOO_FEW_BOMBERS }, 40, 30, 2, 0x53 },
    { { 31,  0, LOW_FUEL }, { 20,  0, TOO_FEW_BOMBERS }, 30, 30, 3, 0x54 } },
  { { { 11, 50, 0x55     }, { 10, 60, 0x56            }, 30, 30, 4, 0x57 },
    { { 11, 60, 0x55     }, { 10, 75, 0x58            }, 30, 20, 5, 0x59 } },
  { { { 11, 20, 0x5a     }, {  3, 40, 0x5b            }, 30, 15, 6, 0x54 },
    { { 11, 30, 0x5c     }, {  5, 50, 0x5d            }, 30, 30, 7, 0x5e } },
};
// clang-format on

auto CurrentSide(DataSegment& data) -> Side& {
  return Record<Side>(data, data.current_side_ptr);
}

// The message the air officer objects with, if any.
auto Objection(std::uint8_t* ds, Objections const& rules, std::uint8_t fuel, std::uint16_t bombers) -> std::optional<std::uint16_t> {
  auto&      data{ Data(ds) };
  auto const skill{ data.air_charm };
  auto const short_of{ [&](Shortage const& rule, std::uint16_t value) { return value < rule.limit && (!rule.roll || static_cast<std::uint8_t>(RandomBelow(ds, 10)) + rule.roll > skill); } };
  if (short_of(rules.fuel, fuel)) return rules.fuel.message;
  if (short_of(rules.bombers, bombers)) return rules.bombers.message;
  if (static_cast<std::uint8_t>(RandomBelow(ds, 20)) + rules.crew_roll > skill && data.air_experience < rules.morale_limit) return POOR_CREWS;
  if (!RandomBelow(ds, rules.chance)) return rules.chance_message;
  return std::nullopt;
}
} // namespace

// May the mission of the given type (0 carpet bombing, 1 attack sortie, 2 strafing run) against a city (1) or open
// ground (0) go ahead? The air officer says why not, or "Understood"; 1 when the mission may proceed.
uint16_t opeu_air_bomb_check(uint8_t ds[65536], HostEvent event, uint16_t, uint8_t city, uint16_t type) {
  auto&         data{ Data(ds) };
  auto&         side{ CurrentSide(data) };
  auto const    fuel{ side.strength };
  auto const    bombers{ side.bombers };
  std::uint16_t text{ 0 };
  if (city < 2 && type < 3)
    if (auto const message{ Objection(ds, OBJECTIONS[type][city], fuel, bombers) }) text = Call(ds, event, LOOKUP, *message);
  if (text) {
    Call(ds, event, MESSAGE, text, data.air_person);
    return 0;
  }
  Call(ds, event, MESSAGE, UNDERSTOOD, data.air_person);
  return 1;
}

// Announces the sortie (hours until the strike, planes sent, mission name) and, once confirmed, stores it as the side's
// air strike. The delay is random(level + 1), one more at night, more in bad weather, less for the lighter missions;
// the planes are the type's base plus a random extra, at most the side's bombers. 1 when the strike was ordered.
uint16_t opeu_air_bomb_order(uint8_t ds[65536], HostEvent event, uint16_t sp, uint8_t x, uint8_t y, uint16_t type) {
  auto&       data{ Data(ds) };
  auto const  text{ Local(sp, 2 + TEXT_LOCAL) };
  auto const  hours{ static_cast<std::uint16_t>(RandomBelow(ds, data.delivery_level + 1) + (opeu_hour_is_night(data.hour) != 0) + static_cast<std::uint16_t>(0x81 - data.air_leadership) / 30 - type
                                                + 2) };
  auto const& flight{ data.bombing_flights[type] };
  auto const  planes{ opeu_min16(CurrentSide(data).bombers, static_cast<std::uint16_t>(flight.base + RandomBelow(ds, flight.range))) };
  auto const  format{ Call(ds, event, LOOKUP, ANNOUNCE) };
  Call(ds, event, FORMAT, text, format, hours, hours == 1 ? HOUR_TEXT : HOURS_TEXT, planes, planes == 1 ? PLANE_TEXT : PLANES_TEXT, data.bombing_names[type]);
  Call(ds, event, MESSAGE, text, data.air_person);
  if (!opeu_air_confirm(ds, event, Local(sp, 2 + ORDER_DEPTH))) return 0;
  auto& strike{ CurrentSide(data).strike };
  strike.x      = x;
  strike.y      = y;
  strike.planes = planes;
  strike.hours  = static_cast<std::uint8_t>(hours);
  strike.turns  = static_cast<std::uint8_t>(strike.hours + 6);
  return 1;
}

// The Bombing Attack command. A pending strike may be cancelled first; without bombers the command ends. Otherwise the
// player picks a site (starting at the view) and a mission type and confirms; a mission the air officer refuses or
// one that is ordered ends the side's turn.
void opeu_air_bomb_command(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto& data{ Data(ds) };
  if (CurrentSide(data).strike.planes) {
    if (!Call(ds, event, CONFIRM, Call(ds, event, LOOKUP, CANCEL_ORDER), data.context_record3)) return;
    CurrentSide(data).strike.planes = 0;
  }
  if (!CurrentSide(data).bombers) {
    Call(ds, event, MESSAGE, Call(ds, event, LOOKUP, NO_BOMBERS), data.context_record3);
    return;
  }
  Call(ds, event, TITLE, BOMB_TITLE);
  auto const at_x{ Local(sp, 2 + SITE_X_LOCAL) };
  auto const at_y{ Local(sp, 2 + SITE_Y_LOCAL) };
  auto&      site_x{ Record<std::uint8_t>(data, at_x) };
  auto&      site_y{ Record<std::uint8_t>(data, at_y) };
  site_x = static_cast<std::uint8_t>(data.viewport_x);
  site_y = static_cast<std::uint8_t>(data.viewport_y);
  Call(ds, event, MESSAGE, SITE_PROMPT, data.context_record3);
  if (!Call(ds, event, PICK, at_x, at_y)) return;
  auto type{ Call(ds, event, DIALOG, Call(ds, event, LOOKUP, CHOOSE_TYPE), 3, data.context_record3, TYPE_MENU) };
  if (type == CANCELED) return;
  --type;
  if (!Call(ds, event, CONFIRM, Call(ds, event, LOOKUP, CONFIRM_MISSION), data.context_record3)) return;
  auto const callee{ Local(sp, 2 + COMMAND_DEPTH) };
  if (!opeu_air_bomb_check(ds, event, callee, opeu_map_code(ds, site_x, site_y) == CITY_TERRAIN, type) || opeu_air_bomb_order(ds, event, callee, site_x, site_y, type))
    data.side_delays[data.current_side] = 1;
}

// The services of the air force's commands (core/airbomb, airdrop, airforce, airhelp, airlift).
uint16_t svc_air(uint8_t* ds, const char* module, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t result{ 0 };
  switch (static_cast<AirService>(id)) {
  case FORMAT:    c2_request_format(ds, args, count); break;
  case MESSAGE:   opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case CONFIRM:   result = opeu_confirm_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case LOOKUP:    result = opeu_message_lookup(ds, svc_msglook, static_cast<short>(Arg(args, count, 0))); break;
  case PRINT:     c2_cursor_print(ds, args, count); break;
  case SPEAKER:   result = opeu_speaker_panel(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case TEXT_FEED: opeu_text_feed(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case VALUE:     {
    auto const scratch{ port_scratch_enter(0x40) };
    result = opeu_choose_value(ds, svc_keyboard, Arg(args, count, 0), Arg(args, count, 1), static_cast<int16_t>(Arg(args, count, 2)), Arg(args, count, 3), scratch);
    port_scratch_leave(0x40);
    break;
  }
  case DIALOG: result = opeu_delay_dialog(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case TITLE:  opeu_army_title(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case PICK:   {
    auto const frame{ port_frame_enter() };
    result = opeu_map_pick_location(ds, svc_mappick, frame, Arg(args, count, 0), Arg(args, count, 1));
    port_frame_leave();
    break;
  }
  case LIST_DIALOG: result = opeu_list_dialog(ds, svc_listdlg, Arg(args, count, 0), static_cast<uint8_t>(Arg(args, count, 1)), static_cast<uint8_t>(Arg(args, count, 2))); break;
  case POSITION:    opeu_army_position(ds, (TextHost*)port_textout_host(), static_cast<short>(Arg(args, count, 0)), static_cast<short>(Arg(args, count, 1))); break;
  case CONTEXT:     opeu_daily_context(ds, svc_context, static_cast<uint8_t>(Arg(args, count, 0))); break;
  case FAR_ACTION:  result = static_cast<uint16_t>(port_call(ds, static_cast<uint32_t>(Arg(args, count, 0)) * 16u + Arg(args, count, 1), nullptr, 0)); break;
  default:          port_unhandled(module, id, count);
  }
  return result;
}

uint16_t svc_airbomb(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_air(ds, "airbomb", id, args, count);
}

auto port_routine_opeu_air_bomb_command(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  auto const sp{ c2_air_sp_enter() };
  opeu_air_bomb_command(ds, svc_airbomb, sp);
  c2_air_leave();
  return 0;
}
