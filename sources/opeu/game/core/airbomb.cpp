// The Bombing Attack command: the player picks a map site and a mission type (carpet bombing, attack sortie,
// strafing run); the air officer may object (low fuel, too few bombers, poor crews, or by chance), otherwise the
// sortie is announced and, once confirmed, stored as the current side's air strike.
#include "core/airbomb.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "core/airhelp.hpp"
#include "core/limits.hpp"
#include "map/grid.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/listdlg.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "map/mappick.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>
#include <optional>

using namespace opeu::game;

namespace {

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

constexpr std::uint8_t  CITY_TERRAIN{ 1 };
constexpr std::uint16_t TEXT_BYTES{ 0x66 }; // the announcement's text, which the format service writes
// The site picked, which the pick service writes: offsets in opeu_air_bomb_command's reserve.
constexpr std::uint16_t SITE_Y_AT{ 0 };
constexpr std::uint16_t SITE_X_AT{ 2 };
constexpr std::uint16_t SITE_BYTES{ 4 };

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

// The message the air officer objects with, if any.
auto Objection(Game& game, Objections const& rules, std::uint8_t fuel, std::uint16_t bombers) -> std::optional<std::uint16_t> {
  auto&      data{ Data(game) };
  auto const skill{ data.air_charm };
  auto const short_of{ [&](Shortage const& rule, std::uint16_t value) { return value < rule.limit && (!rule.roll || static_cast<std::uint8_t>(RandomBelow(game, 10)) + rule.roll > skill); } };
  if (short_of(rules.fuel, fuel)) return rules.fuel.message;
  if (short_of(rules.bombers, bombers)) return rules.bombers.message;
  if (static_cast<std::uint8_t>(RandomBelow(game, 20)) + rules.crew_roll > skill && data.air_experience < rules.morale_limit) return POOR_CREWS;
  if (!RandomBelow(game, rules.chance)) return rules.chance_message;
  return std::nullopt;
}
} // namespace

// May the mission of the given type (0 carpet bombing, 1 attack sortie, 2 strafing run) against a city (1) or open
// ground (0) go ahead? The air officer says why not, or "Understood"; 1 when the mission may proceed.
uint16_t opeu_air_bomb_check(Game& game, uint8_t city, uint16_t type) {
  auto&         data{ Data(game) };
  auto&         side{ CurrentSide(data) };
  auto const    fuel{ side.strength };
  auto const    bombers{ side.bombers };
  std::uint16_t text{ 0 };
  if (city < 2 && type < 3)
    if (auto const message{ Objection(game, OBJECTIONS[type][city], fuel, bombers) }) text = opeu_message_lookup(game, static_cast<short>(*message));
  if (text) {
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text, data.air_person);
    return 0;
  }
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), UNDERSTOOD, data.air_person);
  return 1;
}

// Announces the sortie (hours until the strike, planes sent, mission name) and, once confirmed, stores it as the side's
// air strike. The delay is random(level + 1), one more at night, more in bad weather, less for the lighter missions;
// the planes are the type's base plus a random extra, at most the side's bombers. 1 when the strike was ordered.
uint16_t opeu_air_bomb_order(Game& game, uint8_t x, uint8_t y, uint16_t type) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(game) };
  auto const          text{ locals.At(0) };
  auto const  hours{ static_cast<std::uint16_t>(RandomBelow(game, data.delivery_level + 1) + (opeu_hour_is_night(data.hour) != 0) + static_cast<std::uint16_t>(0x81 - data.air_leadership) / 30 - type
                                                + 2) };
  auto const& flight{ data.bombing_flights[type] };
  auto const  planes{ opeu_min16(CurrentSide(data).bombers, static_cast<std::uint16_t>(flight.base + RandomBelow(game, flight.range))) };
  auto const  format{ opeu_message_lookup(game, static_cast<short>(ANNOUNCE)) };
  opeu_request_format(game, std::array<std::uint16_t, 7>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(format), static_cast<std::uint16_t>(hours),
                                                          static_cast<std::uint16_t>(hours == 1 ? HOUR_TEXT : HOURS_TEXT), static_cast<std::uint16_t>(planes),
                                                          static_cast<std::uint16_t>(planes == 1 ? PLANE_TEXT : PLANES_TEXT), static_cast<std::uint16_t>(data.bombing_names[type]) });
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text, data.air_person);
  if (!opeu_air_confirm(game)) return 0;
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
void opeu_air_bomb_command(Game& game) {
  auto& data{ Data(game) };
  if (CurrentSide(data).strike.planes) {
    if (!opeu_confirm_message(game, static_cast<const DialogHost*>(port_dialogs_host()), opeu_message_lookup(game, static_cast<short>(CANCEL_ORDER)), data.context_record3)) return;
    CurrentSide(data).strike.planes = 0;
  }
  if (!CurrentSide(data).bombers) {
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), opeu_message_lookup(game, static_cast<short>(NO_BOMBERS)), data.context_record3);
    return;
  }
  opeu_army_title(game, static_cast<const DialogHost*>(port_dialogs_host()), BOMB_TITLE);
  ServiceLocals const locals{ SITE_BYTES };
  auto const          at_x{ locals.At(SITE_X_AT) };
  auto const          at_y{ locals.At(SITE_Y_AT) };
  auto&               site_x{ Record<std::uint8_t>(data, at_x) };
  auto&               site_y{ Record<std::uint8_t>(data, at_y) };
  site_x = static_cast<std::uint8_t>(data.viewport_x);
  site_y = static_cast<std::uint8_t>(data.viewport_y);
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), SITE_PROMPT, data.context_record3);
  if (!opeu_map_pick_location(game, at_x, at_y)) return;
  auto type{ opeu_delay_dialog(game, static_cast<const DialogHost*>(port_dialogs_host()), opeu_message_lookup(game, static_cast<short>(CHOOSE_TYPE)), 3, data.context_record3, TYPE_MENU) };
  if (type == MENU_CANCELLED) return;
  --type;
  if (!opeu_confirm_message(game, static_cast<const DialogHost*>(port_dialogs_host()), opeu_message_lookup(game, static_cast<short>(CONFIRM_MISSION)), data.context_record3)) return;
  if (!opeu_air_bomb_check(game, opeu_map_code(game, site_x, site_y) == CITY_TERRAIN, type) || opeu_air_bomb_order(game, site_x, site_y, type)) data.side_delays[data.current_side] = 1;
}

auto port_routine_opeu_air_bomb_command(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_air_bomb_command(game);
  return 0;
}
