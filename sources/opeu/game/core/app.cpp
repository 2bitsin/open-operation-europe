// The game's main loop: years, months, days and hours; each hour every regiment of the shuffled turn order takes its
// turn, after the victory check and followed by the turn's notices. A saved game resumes inside the loop.
#include "core/app.hpp"
#include "core/calendar.hpp"
#include "core/numeric.hpp"
#include "core/turnord.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/appinit.hpp"
#include "core/dispatch.hpp"
#include "core/notend.hpp"
#include "core/notice.hpp"
#include "core/scenend.hpp"
#include "core/victory.hpp"
#include "map/btlend.hpp"
#include "map/btlsetup.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// DataSegment::startup_mode: what start-up asks of the loop.
constexpr std::uint8_t  FIRST_START{ 100 }; // initialise before start-up
constexpr std::uint8_t  RESUME_TURN{ 11 };  // resume at the current regiment's turn
constexpr std::uint8_t  NEW_GAME{ 21 };     // run from the current date
constexpr std::uint8_t  LAST_YEAR{ 10 };    // the calendar stops after the tenth year
constexpr std::uint8_t  TURN_SLOTS{ 64 };
constexpr std::uint16_t PRESENT_ONE{ 1 };

// After the last year (or with a start-up mode it does not know) the game stops: the host keeps running so the window
// still closes and a headless run still stops at its time limit.
[[noreturn]] auto Stop(Game& game) -> void {
  for (;;) game.Machine().Input().Pump();
}
} // namespace

void opeu_application_entry(Game& game) {
  auto& data{ Data(game) };
  if (static_cast<std::uint8_t>(data.startup_mode) == FIRST_START) opeu_app_initialize(game);
  data.startup_mode = 0;
  opeu_app_startup(game);
  auto const mode{ static_cast<std::uint8_t>(data.startup_mode) };
  if (mode != RESUME_TURN && mode != NEW_GAME) Stop(game);
  // Resuming a turn enters the innermost loop as it stands: no loop condition, day start, hour start or turn
  // selection runs before the turn's victory check.
  bool resume{ mode == RESUME_TURN };
  do {
    for (; resume || data.month <= 12; ++data.month) {
      for (; resume || data.day <= opeu_days_in_month(data.year, data.month); ++data.day) {
        if (!resume && data.day == 1) data.schedule_flags[0] |= Bits(ScheduleFlag::MONTHLY);
        for (; resume || data.hour < 24; ++data.hour) {
          if (!resume) opeu_begin_hour(game);
          for (; resume || data.turn_cursor < TURN_SLOTS; ++data.turn_cursor) {
            if (!resume) opeu_select_turn_context(game);
            resume = false;
            if (opeu_check_victory(game)) {
              opeu_app_finish(game);
              return;
            }
            opeu_process_selected_turn(game);
            if (data.turn_notification) {
              opeu_notice_begin(game);
              if (data.presentation_mode == PRESENT_ONE)
                opeu_notice_one(game);
              else
                opeu_notice_other(game);
              opeu_notice_end(game);
            }
          }
        }
        data.hour = 0;
      }
      data.day = 1;
    }
    data.month = 1;
  } while (++data.year < LAST_YEAR);
  Stop(game);
}

// Makes the regiment at the turn cursor current: the slot's byte holds the side (bits 5-7), the division (bits 3-4)
// and the regiment (bits 0-2).
void opeu_select_turn_context(Game& game) {
  auto&      data{ Data(game) };
  auto const slot{ data.turn_order[data.turn_cursor] };
  data.current_side  = static_cast<std::uint8_t>(slot >> 5);
  data.current_group = static_cast<std::uint8_t>((slot & 0x18) >> 3);
  data.current_entry = static_cast<std::uint8_t>(slot & 7);
  auto const group_index{ data.current_side * GROUPS_PER_SIDE + data.current_group };
  data.current_side_ptr  = Near(data, data.sides[data.current_side]);
  data.current_group_ptr = Near(data, data.groups[group_index]);
  data.current_unit_ptr  = Near(data, data.units[group_index * UNITS_PER_GROUP + data.current_entry]);
}

// Starts an hour: schedules the half-day round at 0:00 and 12:00 and the hourly round, counts the sides' delays down,
// shuffles the turn order and puts the cursor at its first slot.
void opeu_begin_hour(Game& game) {
  auto& data{ Data(game) };
  if (data.hour == 0 || data.hour == 12) data.schedule_flags[0] |= Bits(ScheduleFlag::HALF_DAY);
  data.schedule_flags[0] |= Bits(ScheduleFlag::HOURLY);
  data.side_delays[0]     = SubByte(data.side_delays[0], 1);
  data.side_delays[1]     = SubByte(data.side_delays[1], 1);
  opeu_shuffle_turn_order(game);
  data.turn_cursor = 0;
}
