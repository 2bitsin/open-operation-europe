// The turn of the current regiment: the rounds the schedule asks for, the side's commanders in the context panel,
// then (for a ready regiment) the player's or the computer's control of the side, the division's tasks and the
// regiment's final action.
#include "core/dispatch.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/context.hpp"
#include "core/control1.hpp"
#include "core/control2.hpp"
#include "core/daily.hpp"
#include "core/dialogs.hpp"
#include "core/evreport.hpp"
#include "core/grptask.hpp"
#include "core/halfday.hpp"
#include "core/msntick.hpp"
#include "core/normal.hpp"
#include "core/objrank.hpp"
#include "core/people.hpp"
#include "core/selunit.hpp"
#include "core/sideinit.hpp"
#include "io/mouse.hpp"
#include "link/glue.hpp"
#include "map/influenc.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/finalact.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t LABEL_FORMAT{ offsetof(DataSegment, turn_label_format) };

} // namespace

void opeu_process_selected_turn(Game& game) {
  auto& data{ Data(game) };
  opeu_normalize_turn_state(game);
  if (!(data.side_flags[data.current_side] & Bits(SideFlag::TURN_STARTED))) {
    opeu_initialize_turn_side(game);
    data.side_flags[data.current_side] |= Bits(SideFlag::TURN_STARTED);
  }
  auto& schedule{ data.schedule_flags[0] };
  if (schedule & Bits(ScheduleFlag::MONTHLY)) opeu_turn_daily(game);
  if (schedule & Bits(ScheduleFlag::HALF_DAY)) opeu_turn_half_day(game);
  if (schedule & Bits(ScheduleFlag::HOURLY)) {
    opeu_turn_hourly_a(game);
    opeu_turn_hourly_b(game);
    opeu_turn_hourly_c(game);
  }
  opeu_turn_pre_context(game);
  schedule                  = 0;
  data.turn_notification    = 0;
  data.context_record1      = opeu_find_side_record(game, static_cast<uint8_t>(data.current_side), static_cast<uint8_t>(1));
  data.context_record2      = opeu_find_side_record(game, static_cast<uint8_t>(data.current_side), static_cast<uint8_t>(2));
  data.context_record3      = opeu_find_side_record(game, static_cast<uint8_t>(data.current_side), static_cast<uint8_t>(3));
  data.context_group_record = opeu_find_group_record(game, static_cast<uint8_t>(data.current_side), static_cast<uint8_t>(data.current_group));
  if (!(Record<Unit>(data, data.current_unit_ptr).flags & Bits(UnitFlag::READY))) return;
  if (data.side_status[data.current_side] == SIDE_PLAYED) {
    opeu_turn_active_begin(game, static_cast<const DialogHost*>(port_dialogs_host()));
    std::uint16_t const label[]{ LABEL_FORMAT, data.side_names[data.current_side], static_cast<std::uint16_t>(data.current_group * 10 + data.current_entry + 11),
                                 data.entry_names[data.current_entry] };
    opeu_cursor_print(game, label);
    if (opeu_find_side_selected_unit(game, static_cast<uint8_t>(data.current_side)) == data.current_unit_ptr && data.side_control[data.current_side] == SIDE_PLAYED) {
      if (data.input_mode ? static_cast<uint16_t>(opeu_turn_input_a(game)) : opeu_turn_input_b()) data.side_delays[data.current_side] = 0;
    }
  }
  if (opeu_find_side_selected_unit(game, static_cast<uint8_t>(data.current_side)) == data.current_unit_ptr) {
    opeu_turn_selected_begin(game);
    if (data.side_control[data.current_side] == SIDE_PLAYED) {
      if (data.side_delays[data.current_side] == 0) opeu_turn_control_one(game);
    } else
      opeu_turn_control_other(game);
  }
  if (opeu_find_group_selected_unit(game, data.current_group_ptr) == data.current_unit_ptr) opeu_turn_group_action(game);
  if (opeu_turn_final_predicate(game)) opeu_turn_final_action(game);
}
