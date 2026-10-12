// The air-raid event: from 13 June in scenario 3, by day in fair weather, each ready special regiment of side 1 may be
// bombed (1 in 150 - 20 x level, level the delivery level when side 1 is the computer's, else 2). The side the player
// watches gets a notice, a falling bomb over the regiment and the result; the regiment then takes casualties.
//
// Routine frames live on the DS stack: bp is the routine's frame pointer, and a local whose address reaches a
// service sits at Local(bp, n).
#include "core/evbomb.hpp"
#include "common/events.hpp"
#include "core/people.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "common/scenario.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "glue.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/mappixel.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/casualty.hpp"
#include "units/unitname.hpp"

using namespace opeu::game;

namespace {

// The services the raid calls through its event; far pointers are passed as (segment, offset) unless noted.
enum Service : std::uint16_t {
  BITMAP_SAVE    = 0,  // (x, y, w, h, segment, offset)
  DRAW_FAR       = 1,  // (x, y, w, h, mode, segment, offset)
  SPRITE_COMPOSE = 2,  // (x, y, w, h, image offset, segment, mask offset, segment, target offset, segment, mode)
  BITMAP_RESTORE = 3,  // (x, y, w, h, mode, segment, offset)
  MAP_CENTER     = 4,  // (x, y, side)
  VIEW_PIXEL     = 5,  // (&x, &y): a cell to its screen pixel
  SOUND_CUE      = 6,  // ()
  DELAY          = 7,  // (ticks)
  MUSIC_COMMAND7 = 8,  // ()
  PENDING_FINISH = 9,  // (value)
  UNIT_NAME      = 10, // (unit, buffer)
  LOOKUP         = 11, // (message id) -> the text
  REQUEST_FORMAT = 12, // (destination, format, word)
  MESSAGE        = 13, // (text, person)
  DAILY_SHOW     = 14, // (picture, side, presenter offset, presenter segment)
  CASUALTIES     = 15, // (unit, percent, rate, planes, 0, 1)
};

// Message ids (message_lookup): the notice and the result, for side 0 and side 1.
constexpr std::uint16_t RAID_NOTICE_0{ 0x21 };
constexpr std::uint16_t RAID_RESULT_0{ 0x22 };
constexpr std::uint16_t RAID_NOTICE_1{ 0x23 };
constexpr std::uint16_t RAID_RESULT_1{ 0x24 };

constexpr Scenario      RAID_SCENARIO{ Scenario::D_DAY };
constexpr std::uint8_t  RAID_MONTH{ 6 };
constexpr std::uint8_t  RAID_DAY{ 13 };
constexpr std::uint8_t  RAIDED_SIDE{ 1 };
constexpr std::uint8_t  NO_VOICE{ 0xff };
constexpr std::uint8_t  AIR_KIND{ 3 };
constexpr std::uint16_t RAID_PICTURE{ 0x1c7 };
constexpr std::uint16_t RAID_PRESENTER{ 0x0aae }; // opeu_bomb_present, in the code segment below
constexpr std::uint16_t RAID_SEGMENT{ 0x108c };

// The bomb sprite: the background it covers is saved in its own buffer segment; image, sprite and mask in the
// sprite segment.
constexpr std::uint16_t BOMB_SAVE_SEGMENT{ 0x3a99 };
constexpr std::uint16_t BOMB_IMAGE{ 0x1058 };
constexpr std::uint16_t BOMB_SPRITE{ 0x0560 };
constexpr std::uint16_t BOMB_MASK{ 0x0e60 };
constexpr std::uint16_t SHADOW_MODE{ 0x20f };
constexpr std::uint16_t FALL_TOP{ 348 };
constexpr std::uint16_t WHISTLE_ROW{ static_cast<std::uint16_t>(-32) }; // the cue sounds 32 rows below the target

// opeu_bomb_event's frame: the report, the notice, the regiment's name, and the animation's frame pointer.
constexpr std::uint16_t REPORT_LOCAL{ 0xce };
constexpr std::uint16_t INTRO_LOCAL{ 0x72 };
constexpr std::uint16_t NAME_LOCAL{ 0x18 };
constexpr std::uint16_t ANIMATION_FRAME{ 0xd4 };
// opeu_bomb_animate's frame: the regiment's pixel.
constexpr std::uint16_t PIXEL_X_LOCAL{ 2 };
constexpr std::uint16_t PIXEL_Y_LOCAL{ 4 };

constexpr std::uint16_t BOMB_RESERVE{ 0x140 };

auto SetStream(DataSegment& data, std::uint16_t segment, std::uint16_t offset) -> void {
  data.stream = { offset, segment };
}
} // namespace

// Saves the background under the bomb at (x, y) and draws the bomb and its shadow there.
void opeu_bomb_draw_frame(uint8_t ds[65536], HostEvent event, uint16_t load, uint16_t x, uint16_t y) {
  auto& data{ Data(ds) };
  SetStream(data, static_cast<std::uint16_t>(load + BOMB_SAVE_SEGMENT), 0);
  Call(ds, event, BITMAP_SAVE, x, y, 4, 32, data.stream.segment, data.stream.offset);
  SetStream(data, static_cast<std::uint16_t>(load + SPRITE_SEGMENT), BOMB_IMAGE);
  Call(ds, event, DRAW_FAR, x + 1, y + 8, 3, 24, SHADOW_MODE, data.stream.segment, data.stream.offset);
  SetStream(data, static_cast<std::uint16_t>(load + SPRITE_SEGMENT), BOMB_SPRITE);
  Call(ds, event, SPRITE_COMPOSE, x, y, 3, 24, data.stream.offset, data.stream.segment, BOMB_MASK, load + SPRITE_SEGMENT, 0, data.image_segment, 0);
}

// Puts the background under the bomb at (x, y) back.
void opeu_bomb_restore_frame(uint8_t ds[65536], HostEvent event, uint16_t load, uint16_t x, uint16_t y) {
  auto& data{ Data(ds) };
  SetStream(data, static_cast<std::uint16_t>(load + BOMB_SAVE_SEGMENT), 0);
  Call(ds, event, BITMAP_RESTORE, x, y, 4, 32, 0, data.stream.segment, data.stream.offset);
}

// Centres the map on the regiment and drops the bomb from the top of the screen onto it.
void opeu_bomb_animate(uint8_t ds[65536], HostEvent event, uint16_t load, uint16_t bp, uint16_t unit, uint8_t side) {
  auto&       data{ Data(ds) };
  auto const& target{ Record<Unit>(data, unit) };
  auto const  ux{ static_cast<std::uint16_t>(target.x) };
  auto const  uy{ static_cast<std::uint16_t>(target.y) };
  data.map_mode[0] = 1;
  Call(ds, event, MAP_CENTER, static_cast<std::uint8_t>(ux), static_cast<std::uint8_t>(uy), opeu_side_for_status(ds, side));
  auto const pixel_x{ Local(bp, PIXEL_X_LOCAL) };
  auto const pixel_y{ Local(bp, PIXEL_Y_LOCAL) };
  SetWord(ds, pixel_x, ux);
  SetWord(ds, pixel_y, uy);
  Call(ds, event, VIEW_PIXEL, pixel_x, pixel_y);
  SetWord(ds, pixel_y, static_cast<std::uint16_t>(Word(ds, pixel_y) + 8));
  for (std::uint16_t row{ FALL_TOP }; row >= Word(ds, pixel_y); --row) {
    opeu_bomb_draw_frame(ds, event, load, Word(ds, pixel_x), row);
    if (static_cast<std::uint16_t>(Word(ds, pixel_y) - row) == WHISTLE_ROW) Call(ds, event, SOUND_CUE);
    Call(ds, event, DELAY, 1);
    opeu_bomb_restore_frame(ds, event, load, Word(ds, pixel_x), row);
  }
  Call(ds, event, MUSIC_COMMAND7);
}

// The daily-show presenter of the raid.
void opeu_bomb_present(uint8_t ds[65536], HostEvent event, uint16_t) {
  Call(ds, event, PENDING_FINISH, 3);
}

void opeu_bomb_event(uint8_t ds[65536], HostEvent event, uint16_t load, uint16_t bp) {
  auto& data{ Data(ds) };
  if (CurrentScenario(data) != RAID_SCENARIO) return;
  if (data.month < RAID_MONTH || (data.month == RAID_MONTH && data.day < RAID_DAY)) return;
  if (!opeu_hour_is_day(data.hour)) return;
  if (data.weather != 0 && data.weather != 1) return;
  auto const report{ Local(bp, REPORT_LOCAL) };
  auto const intro{ Local(bp, INTRO_LOCAL) };
  auto const name{ Local(bp, NAME_LOCAL) };
  auto const level{ static_cast<std::uint16_t>(data.side_control[RAIDED_SIDE] == 1 ? data.delivery_level : 2) };
  for (unsigned group{ 0 }; group < GROUPS_PER_SIDE; ++group) {
    for (auto& regiment : GroupUnits(data, RAIDED_SIDE, group)) {
      auto const troop{ Near(data, regiment) };
      if (!(regiment.flags & Bits(UnitFlag::READY)) || !opeu_unit_special(ds, troop) || RandomBelow(ds, static_cast<std::uint16_t>(150 - level * 20))) continue;
      Call(ds, event, UNIT_NAME, troop, name);
      std::uint8_t voice{ NO_VOICE };
      if (data.side_status[0] == SIDE_PLAYED) {
        voice = 0;
        opeu_string_copy(ds, intro, Call(ds, event, LOOKUP, RAID_NOTICE_0));
        Call(ds, event, REQUEST_FORMAT, report, Call(ds, event, LOOKUP, RAID_RESULT_0), name);
      } else if (data.side_status[1] == SIDE_PLAYED && opeu_unit_selection_eligible(ds, troop)) {
        voice = 1;
        opeu_string_copy(ds, intro, Call(ds, event, LOOKUP, RAID_NOTICE_1));
        Call(ds, event, REQUEST_FORMAT, report, Call(ds, event, LOOKUP, RAID_RESULT_1), name);
      }
      auto const saved_mode{ data.map_mode[0] };
      if (voice != NO_VOICE) {
        auto const person{ opeu_find_side_record(ds, voice, AIR_KIND) };
        Call(ds, event, MESSAGE, intro, person);
        Call(ds, event, DAILY_SHOW, RAID_PICTURE, 0, RAID_PRESENTER, load + RAID_SEGMENT);
        opeu_bomb_animate(ds, event, load, Local(bp, ANIMATION_FRAME), troop, voice);
        Call(ds, event, MESSAGE, report, person);
      }
      Call(ds, event, CASUALTIES, troop, 15, 30, 10, 0, 1);
      data.map_mode[0] = saved_mode;
    }
  }
}

uint16_t svc_evbomb(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t   result{ 0 };
  auto const word{ [&](unsigned i) { return Arg(args, count, i); } };
  switch (id) {
  case BITMAP_SAVE:    opeu_bitmap_save(word(0), word(1), word(2), word(3), port_far(word(4), word(5))); break;
  case DRAW_FAR:       result = static_cast<uint16_t>(opeu_draw_far(word(0), word(1), word(2), word(3), word(4), port_far(word(5), word(6)))); break;
  case SPRITE_COMPOSE: opeu_sprite_compose(word(0), word(1), word(2), word(3), port_far(word(5), word(4)), port_far(word(7), word(6)), port_far(word(9), word(8)), word(10)); break;
  case BITMAP_RESTORE: result = static_cast<uint16_t>(opeu_bitmap_restore(word(0), word(1), word(2), word(3), word(4), port_far(word(5), word(6)))); break;
  case MAP_CENTER:     {
    auto const frame{ port_frame_enter() };
    opeu_map_center(ds, svc_mappos, frame, static_cast<uint8_t>(word(0)), static_cast<uint8_t>(word(1)), static_cast<uint8_t>(word(2)));
    port_frame_leave();
    break;
  }
  case VIEW_PIXEL:     opeu_view_pixel(ds, word(0), word(1)); break;
  case SOUND_CUE:      opeu_sound_cue4(ds, svc_soundcue); break;
  case DELAY:          opeu_delay_ticks(ds, svc_waitkey, word(0)); break;
  case MUSIC_COMMAND7: opeu_music_command7(ds, svc_sound); break;
  case PENDING_FINISH: opeu_pending_finish(ds, svc_waitkey, static_cast<uint8_t>(word(0))); break;
  case UNIT_NAME:      result = opeu_unit_name(ds, svc_unitname, word(0), word(1)); break;
  case LOOKUP:         result = opeu_message_lookup(ds, svc_msglook, static_cast<int16_t>(word(0))); break;
  case REQUEST_FORMAT: c1_request_format(ds, args, count); break;
  case MESSAGE:        opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), word(0), word(1)); break;
  case DAILY_SHOW:     port_daily_show(ds, word(0), word(1), word(2), word(3)); break;
  case CASUALTIES:     {
    auto const frame{ port_frame_enter() };
    opeu_unit_casualties(ds, svc_casualty, frame, word(0), word(1), word(2), word(3), word(4), word(5));
    port_frame_leave();
    break;
  }
  default: port_unhandled("evbomb", id, count);
  }
  return result;
}

auto port_routine_opeu_bomb_event(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  auto const sp{ port_stack_enter(BOMB_RESERVE) };
  opeu_bomb_event(ds, svc_evbomb, 0, static_cast<std::uint16_t>(sp - 2));
  port_stack_leave(BOMB_RESERVE);
  return 0;
}

auto port_routine_opeu_bomb_present(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_bomb_present(ds, svc_evbomb, 0);
  return 0;
}
