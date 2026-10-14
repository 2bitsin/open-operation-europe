// The air-raid event: from 13 June in scenario 3, by day in fair weather, each ready special regiment of side 1 may be
// bombed (1 in 150 - 20 x level, level the delivery level when side 1 is the computer's, else 2). The side the player
// watches gets a notice, a falling bomb over the regiment and the result; the regiment then takes casualties.
//
// A local whose address reaches a service lives in the routine's ServiceLocals reserve.
#include "core/evbomb.hpp"
#include "common/air-flight.hpp"
#include "common/events.hpp"
#include "core/people.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/mappixel.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/casualty.hpp"
#include "units/unitname.hpp"
#include <array>

using namespace opeu::game;

namespace {

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
constexpr std::uint16_t RAID_PICTURE{ 0x1c7 };

// The bomb sprite: the background it covers is saved in its own buffer segment; image, sprite and mask in the
// sprite segment.
constexpr std::uint16_t BOMB_SAVE_SEGMENT{ 0x3a99 };
constexpr std::uint16_t BOMB_IMAGE{ 0x1058 };
constexpr std::uint16_t BOMB_SPRITE{ 0x0560 };
constexpr std::uint16_t BOMB_MASK{ 0x0e60 };
constexpr std::uint16_t SHADOW_MODE{ 0x20f };
constexpr std::uint16_t FALL_TOP{ 348 };
constexpr std::uint16_t WHISTLE_ROW{ static_cast<std::uint16_t>(-32) }; // the cue sounds 32 rows below the target

// opeu_bomb_event's texts, which services write and show: offsets in its reserve.
constexpr std::uint16_t REPORT_AT{ 0x00 }; // 92 bytes
constexpr std::uint16_t INTRO_AT{ 0x5c };  // 90 bytes
constexpr std::uint16_t NAME_AT{ 0xb6 };   // 24 bytes
constexpr std::uint16_t EVENT_LOCALS{ 0xce };

} // namespace

// Saves the background under the bomb at (x, y) and draws the bomb and its shadow there.
void opeu_bomb_draw_frame(Game& game, uint16_t load, uint16_t x, uint16_t y) {
  auto& data{ Data(game) };
  SetStream(data, static_cast<std::uint16_t>(load + BOMB_SAVE_SEGMENT), 0);
  game.Machine().Display().SaveColumns({ x, y, 4, 32 }, FarBytes(game, data.stream.segment, data.stream.offset));
  SetStream(data, static_cast<std::uint16_t>(load + SPRITE_SEGMENT), BOMB_IMAGE);
  game.Machine().Display().DrawBits({ static_cast<uint16_t>(x + 1), static_cast<uint16_t>(y + 8), 3, 24 }, { SHADOW_MODE, FarBytes(game, data.stream.segment, data.stream.offset) });
  SetStream(data, static_cast<std::uint16_t>(load + SPRITE_SEGMENT), BOMB_SPRITE);
  game.Machine().Display().ComposeSprite({ x, y, 3, 24 }, { .and_mask = FarBytes(game, data.stream.segment, data.stream.offset),
                                                            .or_image = FarBytes(game, load + SPRITE_SEGMENT, BOMB_MASK),
                                                            .buffer   = FarBytes(game, data.image_segment, 0),
                                                            .flags    = 0 });
}

// Puts the background under the bomb at (x, y) back.
void opeu_bomb_restore_frame(Game& game, uint16_t load, uint16_t x, uint16_t y) {
  auto& data{ Data(game) };
  SetStream(data, static_cast<std::uint16_t>(load + BOMB_SAVE_SEGMENT), 0);
  game.Machine().Display().DrawColumns({ x, y, 4, 32 }, { 0, FarBytes(game, data.stream.segment, data.stream.offset) });
}

// Centres the map on the regiment and drops the bomb from the top of the screen onto it.
void opeu_bomb_animate(Game& game, uint16_t load, uint16_t unit, uint8_t side) {
  auto&       data{ Data(game) };
  auto const& target{ Record<Unit>(data, unit) };
  auto const  ux{ static_cast<std::uint16_t>(target.x) };
  auto const  uy{ static_cast<std::uint16_t>(target.y) };
  data.map_mode[0] = 1;
  opeu_map_center(game, ux, uy, static_cast<std::uint16_t>(opeu_side_for_status(game, side)));
  auto const pixel{ ViewPixel(Data(game), { static_cast<std::uint8_t>(ux), static_cast<std::uint8_t>(uy) }) };
  auto const target_y{ static_cast<std::uint16_t>(pixel.y + 8) };
  for (std::uint16_t row{ FALL_TOP }; row >= target_y; --row) {
    opeu_bomb_draw_frame(game, load, pixel.x, row);
    if (static_cast<std::uint16_t>(target_y - row) == WHISTLE_ROW) opeu_sound_cue4(game);
    opeu_delay_ticks(game, 1);
    opeu_bomb_restore_frame(game, load, pixel.x, row);
  }
  opeu_music_command7(game);
}

// The daily-show presenter of the raid.
void opeu_bomb_present(Game& game) {
  opeu_pending_finish(game, 3);
}

void opeu_bomb_event(Game& game, uint16_t load) {
  auto& data{ Data(game) };
  if (CurrentScenario(data) != RAID_SCENARIO) return;
  if (data.month < RAID_MONTH || (data.month == RAID_MONTH && data.day < RAID_DAY)) return;
  if (!opeu_hour_is_day(data.hour)) return;
  if (data.weather != 0 && data.weather != 1) return;
  ServiceLocals const locals{ EVENT_LOCALS };
  auto const          report{ locals.At(REPORT_AT) };
  auto const          intro{ locals.At(INTRO_AT) };
  auto const          name{ locals.At(NAME_AT) };
  auto const          level{ static_cast<std::uint16_t>(data.side_control[RAIDED_SIDE] == SIDE_PLAYED ? data.delivery_level : 2) };
  for (unsigned group{ 0 }; group < GROUPS_PER_SIDE; ++group) {
    for (auto& regiment : GroupUnits(data, RAIDED_SIDE, group)) {
      auto const troop{ Near(data, regiment) };
      if (!(regiment.flags & Bits(UnitFlag::READY)) || !opeu_unit_special(game, troop) || game.State().RandomBounded(static_cast<std::uint16_t>(150 - level * 20))) continue;
      opeu_unit_name(game, troop, name);
      std::uint8_t voice{ NO_VOICE };
      if (data.side_status[0] == SIDE_PLAYED) {
        voice = 0;
        opeu_string_copy(game, intro, opeu_message_lookup(game, RAID_NOTICE_0));
        opeu_request_format(game, report, std::array<FormatArgument, 2>{ opeu_message_lookup(game, RAID_RESULT_0), name });
      } else if (data.side_status[1] == SIDE_PLAYED && opeu_unit_selection_eligible(game, troop)) {
        voice = 1;
        opeu_string_copy(game, intro, opeu_message_lookup(game, RAID_NOTICE_1));
        opeu_request_format(game, report, std::array<FormatArgument, 2>{ opeu_message_lookup(game, RAID_RESULT_1), name });
      }
      auto const saved_mode{ data.map_mode[0] };
      if (voice != NO_VOICE) {
        auto const person{ opeu_find_side_record(game, voice, AIR_OFFICER_KIND) };
        opeu_control_message(game, intro, person);
        opeu_daily_show(game, RAID_PICTURE, 0, Presenter::BOMBING);
        opeu_bomb_animate(game, load, troop, voice);
        opeu_control_message(game, report, person);
      }
      opeu_unit_casualties(game, troop, { .chance = 15, .base = 30, .spread = 10, .heavy = false, .blast = true });
      data.map_mode[0] = saved_mode;
    }
  }
}
