// The paratroop drop: counting a pending drop down to its hour, flying the transports over the target, landing the
// paratroop regiment, and calling the drop off.
#include "core/paradrop.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "types.hpp"

#include <cstddef>
#include <utility>

#include "core/airshow.hpp"
#include "core/airstrk.hpp"
#include "core/dialogs.hpp"
#include "core/msnrept.hpp"
#include "core/paraunit.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "core/template.hpp"
#include "glue.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/maperase.hpp"
#include "map/mappixel.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// The services the drop routines call through their event.
enum class DropService : uint16_t {
  UNIT_GROUP      = 0, // (unit) -> the unit's division
  CELL_OK         = 1, // (x, y): paratroops may land there
  AIR_REPORT      = 2, // (hours, text) -> 0 fly now, 1 wait an hour, 2 call it off
  SIDE_RECORD     = 3, // (side, role) -> the side's general in the role
  PERSON_TEMPLATE = 4, // (nation mask, kind)
  OPPOSITE_SIDE   = 5, // (side)
  MESSAGE         = 6, // (text, person): a general's message
  CUE             = 7,
  SIDE_FOR_STATUS = 8,  // (side): the colour side of a side's report
  MAP_CENTER      = 9,  // (x, y, side)
  VIEW_PIXEL      = 10, // (&x, &y): a map cell to its screen pixel, in place
  DAILY_SHOW      = 11, // (id, side, presenter offset, presenter segment)
  MUSIC           = 12,
  UNIT_CREATE     = 13, // (side, drop) -> the paratroop regiment
  LAND            = 14, // (unit, losses) -> nonzero when it is still ready
  ARRIVAL_PREPARE = 15, // (unit)
  DELAY           = 16, // (ticks)
  SPRITE_DRAW     = 17, // (x, y, side)
  SPRITE_RESTORE  = 18, // (x, y)
  DAMAGE_ROLL     = 19, // (x, y, planes / 30) -> planes shot down
  DESTROYED       = 20, // (planes' name)
  OFFICER_GAIN    = 21, // (side, kind)
};

// AIR_REPORT's answers.
enum class AirAnswer : uint8_t {
  FLY      = 0,
  WAIT     = 1,
  CALL_OFF = 2,
};

// The drop is announced by the army commander's nation's air force commander, the enemy planes are reported by the
// other side's intelligence chief, and the intelligence chief gains experience from a drop that brings planes home.
constexpr auto COMMANDER{ std::to_underlying(StaffRole::ARMY_COMMANDER) };
constexpr auto AIR_COMMANDER{ std::to_underlying(StaffRole::AIR_FORCE_COMMANDER) };
constexpr auto INTELLIGENCE{ std::to_underlying(StaffRole::INTELLIGENCE_CHIEF) };

// The transports' flight: from pixel row 0x15c up to 0x54; with every plane lost below row 0x114 the flight ends.
constexpr uint16_t FLIGHT_START{ 0x15c };
constexpr uint16_t FLIGHT_END{ 0x54 };
constexpr uint16_t ALL_LOST_ROW{ 0x114 };
// The regiment lands this many rows after the planes pass over the target.
constexpr int LANDING_ROWS{ 0x20 };
// Anti-aircraft fire counts at most this many planes.
constexpr uint16_t FIRE_PLANES{ 0x59 };
constexpr uint16_t SHOW_ID{ 0xfa };
// The far address of opeu_para_drop_present: its offset, and its segment relative to the caller's.
constexpr uint16_t PRESENTER_OFFSET{ 0xd72 };
constexpr uint16_t PRESENTER_SEGMENT{ 0xf43 };
// The pixel words view_pixel converts in place: frame locals of the drop event.
constexpr uint16_t PIXEL_X_LOCAL{ 2 };
constexpr uint16_t PIXEL_Y_LOCAL{ 6 };

} // namespace

// Calls off the air side's drop: no transports, and the regiments and battalions set aside are released.
void opeu_para_clear(uint8_t ds[65536]) {
  auto&      data{ Data(ds) };
  auto const side{ data.air_side };
  data.sides[side].drop.transports = 0;
  for (uint8_t i = 0; i < UNITS_PER_SIDE; i++) {
    auto& unit{ data.units[side * UNITS_PER_SIDE + i] };
    if (unit.flags & Bits(UnitFlag::AIRBORNE)) unit.flags = 0;
  }
  for (uint8_t i = 0; i < ELEMENTS_PER_SIDE; i++) {
    auto& element{ data.elements[side * ELEMENTS_PER_SIDE + i] };
    if (element.flags & Bits(ElementFlag::AIRBORNE)) element.flags = 0;
  }
}

// Whether the paratroop regiment's division is active: nonzero when it is, and also when no regiment is set aside.
uint16_t opeu_para_ready(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto const side{ data.air_side };
  for (uint8_t i = 0; i < UNITS_PER_SIDE; i++) {
    auto const& unit{ data.units[side * UNITS_PER_SIDE + i] };
    if (unit.flags & Bits(UnitFlag::AIRBORNE)) {
      auto const group{ Call(ds, event, DropService::UNIT_GROUP, Near(data, unit)) };
      return (Record<Group>(data, group).flags & Bits(GroupFlag::READY)) ? 1 : 0;
    }
  }
  return 1;
}

// Counts the air side's drop down an hour. A drop whose regiment is not ready or whose target cell no longer takes
// paratroops is called off; when its hour comes the air command decides: fly now (returns 1), hold it an hour, or
// call it off.
uint16_t opeu_para_drop_due(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  auto& drop{ data.sides[data.air_side].drop };
  if (!drop.transports) return 0;
  opeu_sub_byte(1, &drop.due);
  if (!opeu_para_ready(ds, event) || !Call(ds, event, DropService::CELL_OK, drop.x, drop.y))
    opeu_para_clear(ds);
  else if (!opeu_sub_byte(1, &drop.hours))
    switch (static_cast<AirAnswer>(Call(ds, event, DropService::AIR_REPORT, drop.due, Near(data, data.paratroop_drop_name)) & 0xff)) {
    case AirAnswer::FLY:      return 1;
    case AirAnswer::WAIT:     drop.hours = 1; break;
    case AirAnswer::CALL_OFF: opeu_para_clear(ds); break;
    }
  return 0;
}

// The report shown when the transports reach the target: the side's air commander announces the drop to a player's
// side, the other side's intelligence chief reports the enemy planes.
void opeu_para_drop_present(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto const side{ data.air_side };
  if (data.side_status[side] == SIDE_PLAYED) {
    auto const commander{ Call(ds, event, DropService::SIDE_RECORD, side, COMMANDER) };
    auto const person{ Call(ds, event, DropService::PERSON_TEMPLATE, Record<Person>(data, commander).status, AIR_COMMANDER) };
    Call(ds, event, DropService::MESSAGE, Near(data, data.paratroop_drop_begin), person);
  } else {
    auto const other{ static_cast<uint8_t>(Call(ds, event, DropService::OPPOSITE_SIDE, side)) };
    Call(ds, event, DropService::MESSAGE, Near(data, data.paratroop_drop_spotted), Call(ds, event, DropService::SIDE_RECORD, other, INTELLIGENCE));
  }
  Call(ds, event, DropService::CUE);
}

// Flies the air side's drop: centres the map on the target, books the transports and the paratroops out of the
// side's totals, takes the anti-aircraft losses, and moves the formation up the screen. Over the target the drop's
// report is shown; a little later the paratroop regiment is formed and lands with the losses of the planes shot
// down. The surviving transports come back to the side, and the drop is cleared. bp is the caller's frame, where the
// target's pixel position is converted.
void opeu_para_drop_event(uint8_t ds[65536], HostEvent event, uint16_t segment, uint16_t bp) {
  auto&      data{ Data(ds) };
  auto const side_index{ data.air_side };
  auto&      side{ data.sides[side_index] };
  auto&      drop{ side.drop };
  auto const x{ drop.x };
  auto const y{ drop.y };
  auto const was{ data.map_mode[0] };
  data.map_mode[0] = 1;
  Call(ds, event, DropService::MAP_CENTER, x, y, Call(ds, event, DropService::SIDE_FOR_STATUS, side_index));
  uint16_t left{ drop.transports };
  opeu_sub_word(left, &side.planes);
  opeu_sub_word(drop.paratroops, &side.paratroops);
  opeu_sub_word(Call(ds, event, DropService::DAMAGE_ROLL, x, y, opeu_min16(left, FIRE_PLANES) / 30), &left);
  auto const pixel_x{ Local(bp, PIXEL_X_LOCAL) };
  auto const pixel_y{ Local(bp, PIXEL_Y_LOCAL) };
  SetWord(ds, pixel_x, x);
  SetWord(ds, pixel_y, y);
  Call(ds, event, DropService::VIEW_PIXEL, pixel_x, pixel_y);
  uint16_t const target_x{ Word(ds, pixel_x) };
  uint16_t const target_y{ Word(ds, pixel_y) };
  for (uint16_t row = FLIGHT_START; row >= FLIGHT_END; row--) {
    if (row <= ALL_LOST_ROW && left == 0) {
      Call(ds, event, DropService::DESTROYED, Near(data, data.paratroop_planes_name));
      break;
    }
    Call(ds, event, DropService::SPRITE_DRAW, target_x, row, side_index);
    if (row == target_y) Call(ds, event, DropService::DAILY_SHOW, SHOW_ID, Call(ds, event, DropService::SIDE_FOR_STATUS, side_index), PRESENTER_OFFSET, segment + PRESENTER_SEGMENT);
    if (target_y - row == LANDING_ROWS) {
      Call(ds, event, DropService::MUSIC);
      auto const unit{ Call(ds, event, DropService::UNIT_CREATE, side_index, Near(data, drop)) };
      if (Call(ds, event, DropService::LAND, unit, (drop.transports - left) * 10)) Call(ds, event, DropService::ARRIVAL_PREPARE, unit);
    }
    Call(ds, event, DropService::DELAY, 1);
    Call(ds, event, DropService::SPRITE_RESTORE, target_x, row);
  }
  data.map_mode[0] = was;
  if (left) {
    Call(ds, event, DropService::OFFICER_GAIN, side_index, INTELLIGENCE);
    side.pending_planes += left;
  }
  opeu_para_clear(ds);
  drop.transports = 0;
}

// The AIRSHOW and AIRSTRK routines are called with load 0; their bp is the callee's frame.
uint16_t svc_paradrop(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r{ 0 };
  switch (static_cast<DropService>(id)) {
  case DropService::UNIT_GROUP: r = opeu_unit_group(Arg(args, count, 0)); break;
  case DropService::CELL_OK:    r = opeu_para_cell_ok(ds, svc_paraunit, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case DropService::AIR_REPORT: {
    auto const frame{ port_frame_enter() };
    r = opeu_air_report(ds, svc_airshow, frame, Arg(args, count, 1), (uint8_t)Arg(args, count, 0));
    port_frame_leave();
    break;
  }
  case DropService::SIDE_RECORD:     r = opeu_find_side_record(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case DropService::PERSON_TEMPLATE: r = opeu_find_person_template(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case DropService::OPPOSITE_SIDE:   r = opeu_opposite_side((uint8_t)Arg(args, count, 0)); break;
  case DropService::MESSAGE:         opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case DropService::CUE:             opeu_sound_cue4(ds, svc_soundcue); break;
  case DropService::SIDE_FOR_STATUS: r = opeu_side_for_status(ds, (uint8_t)Arg(args, count, 0)); break;
  case DropService::MAP_CENTER:      {
    auto const frame{ port_frame_enter() };
    opeu_map_center(ds, svc_mappos, frame, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2));
    port_frame_leave();
    break;
  }
  case DropService::VIEW_PIXEL:  opeu_view_pixel(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case DropService::DAILY_SHOW:  port_daily_show(ds, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case DropService::MUSIC:       opeu_music_command7(ds, svc_sound); break;
  case DropService::UNIT_CREATE: {
    auto const& drop{ Record<AirDrop>(Data(ds), Arg(args, count, 1)) };
    r = opeu_para_unit_create(ds, svc_paraunit, (uint8_t)Arg(args, count, 0), drop.x, drop.y);
    break;
  }
  case DropService::LAND:            r = opeu_para_land(ds, svc_paraunit, Arg(args, count, 0), Arg(args, count, 1)); break;
  case DropService::ARRIVAL_PREPARE: {
    auto const frame{ port_frame_enter() };
    opeu_arrival_prepare(ds, svc_maperase, frame, Arg(args, count, 0));
    port_frame_leave();
    break;
  }
  case DropService::DELAY:       opeu_delay_ticks(ds, svc_waitkey, Arg(args, count, 0)); break;
  case DropService::SPRITE_DRAW: {
    auto const frame{ port_frame_enter() };
    opeu_air_sprite_draw(ds, svc_airshow, 0, frame, Arg(args, count, 0), Arg(args, count, 1), (uint8_t)Arg(args, count, 2));
    port_frame_leave();
    break;
  }
  case DropService::SPRITE_RESTORE: opeu_air_sprite_restore(ds, svc_airshow, 0, Arg(args, count, 0), Arg(args, count, 1)); break;
  case DropService::DAMAGE_ROLL:    r = opeu_air_damage_roll(ds, svc_airstrk, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case DropService::DESTROYED:      {
    auto const frame{ port_frame_enter() };
    opeu_air_destroyed_message(ds, svc_airstrk, frame, Arg(args, count, 0));
    port_frame_leave();
    break;
  }
  case DropService::OFFICER_GAIN: opeu_officer_gain(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  default:                        port_unhandled("paradrop", id, count);
  }
  return r;
}

auto port_routine_opeu_para_drop_present(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_para_drop_present(ds, svc_paradrop);
  return 0;
}
