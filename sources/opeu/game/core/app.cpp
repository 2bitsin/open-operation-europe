// The game's main loop: years, months, days and hours; each hour every regiment of the shuffled turn order takes its
// turn, after the victory check and followed by the turn's notices. A saved game resumes inside the loop.
#include "core/app.hpp"
#include "core/calendar.hpp"
#include "core/numeric.hpp"
#include "core/random.hpp"
#include "core/turnord.hpp"
#include "data/mapstate.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/appinit.hpp"
#include "core/dispatch.hpp"
#include "core/notend.hpp"
#include "core/notice.hpp"
#include "core/scenend.hpp"
#include "core/victory.hpp"
#include "glue.hpp"
#include "map/btlend.hpp"
#include "map/btlsetup.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstring>

using namespace opeu::game;

namespace {

// The services the loop calls through its event.
enum Service : std::uint16_t {
  INITIALIZE   = 0, // ()
  STARTUP      = 1, // ()
  TURN         = 2, // () the current regiment's turn
  NOTICE_BEGIN = 3, // ()
  NOTICE_ONE   = 4, // () the notice of the player's presentation
  NOTICE_OTHER = 5, // () any other presentation
  NOTICE_END   = 6, // ()
  VICTORY      = 7, // () -> nonzero when the scenario is decided
  FINISH       = 8, // ()
};

// DataSegment::startup_mode: what start-up asks of the loop.
constexpr std::uint8_t  FIRST_START{ 100 }; // initialise before start-up
constexpr std::uint8_t  RESUME_TURN{ 11 };  // resume at the current regiment's turn
constexpr std::uint8_t  NEW_GAME{ 21 };     // run from the current date
constexpr std::uint8_t  LAST_YEAR{ 10 };    // the calendar stops after the tenth year
constexpr std::uint8_t  TURN_SLOTS{ 64 };
constexpr std::uint16_t PRESENT_ONE{ 1 };

// Shuffles the turn order with the segment's random state.
auto ShuffleTurnOrder(DataSegment& data) -> void {
  std::memcpy(&opeu_random_state, data.random_state.data(), sizeof opeu_random_state);
  std::memcpy(opeu_turn_order, data.turn_order.data(), data.turn_order.size());
  opeu_shuffle_turn_order();
  std::memcpy(data.turn_order.data(), opeu_turn_order, data.turn_order.size());
  std::memcpy(data.random_state.data(), &opeu_random_state, sizeof opeu_random_state);
}

// After the last year (or with a start-up mode it does not know) the game stops: the host keeps running so the window
// still closes and a headless run still stops at its time limit.
[[noreturn]] auto Stop() -> void {
  for (;;) port_pump();
}
} // namespace

void opeu_application_entry(uint8_t ds[65536], uint16_t (*event)(uint8_t*, uint16_t)) {
  auto& data{ Data(ds) };
  if (static_cast<std::uint8_t>(data.startup_mode) == FIRST_START) event(ds, INITIALIZE);
  data.startup_mode = 0;
  event(ds, STARTUP);
  auto const mode{ static_cast<std::uint8_t>(data.startup_mode) };
  if (mode != RESUME_TURN && mode != NEW_GAME) Stop();
  // Resuming a turn enters the innermost loop as it stands: no loop condition, day start, hour start or turn
  // selection runs before the turn's victory check.
  bool resume{ mode == RESUME_TURN };
  do {
    for (; resume || data.month <= 12; ++data.month) {
      for (; resume || data.day <= opeu_days_in_month(data.year, data.month); ++data.day) {
        if (!resume && data.day == 1) data.schedule_flags[0] |= Bits(ScheduleFlag::MONTHLY);
        for (; resume || data.hour < 24; ++data.hour) {
          if (!resume) opeu_begin_hour(ds);
          for (; resume || data.turn_cursor < TURN_SLOTS; ++data.turn_cursor) {
            if (!resume) opeu_select_turn_context(ds);
            resume = false;
            if (event(ds, VICTORY)) {
              event(ds, FINISH);
              return;
            }
            event(ds, TURN);
            if (data.turn_notification) {
              event(ds, NOTICE_BEGIN);
              event(ds, data.presentation_mode == PRESENT_ONE ? NOTICE_ONE : NOTICE_OTHER);
              event(ds, NOTICE_END);
            }
          }
        }
        data.hour = 0;
      }
      data.day = 1;
    }
    data.month = 1;
  } while (++data.year < LAST_YEAR);
  Stop();
}

// Makes the regiment at the turn cursor current: the slot's byte holds the side (bits 5-7), the division (bits 3-4)
// and the regiment (bits 0-2).
void opeu_select_turn_context(uint8_t ds[65536]) {
  auto&      data{ Data(ds) };
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
void opeu_begin_hour(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
  if (data.hour == 0 || data.hour == 12) data.schedule_flags[0] |= Bits(ScheduleFlag::HALF_DAY);
  data.schedule_flags[0] |= Bits(ScheduleFlag::HOURLY);
  opeu_sub_byte(1, &data.side_delays[0]);
  opeu_sub_byte(1, &data.side_delays[1]);
  ShuffleTurnOrder(data);
  data.turn_cursor = 0;
}

uint16_t svc_app(uint8_t* ds, uint16_t id) {
  uint16_t result{ 0 };
  switch (id) {
  case INITIALIZE:   opeu_app_initialize(ds, svc_appinit); break;
  case STARTUP:      opeu_app_startup(ds, svc_appinit); break;
  case TURN:         opeu_process_selected_turn(ds, svc_dispatch); break;
  case NOTICE_BEGIN: opeu_notice_begin(ds, svc_notice); break;
  case NOTICE_ONE:   opeu_notice_one(ds, svc_btlsetup); break;
  case NOTICE_OTHER: opeu_notice_other(ds, svc_btlend); break;
  case NOTICE_END:   opeu_notice_end(ds, svc_notend); break;
  case VICTORY:      result = opeu_check_victory(ds); break;
  case FINISH:       opeu_app_finish(ds, svc_scenend); break;
  default:           port_unhandled("app", id, 0);
  }
  return result;
}
