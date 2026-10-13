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

using Event = uint16_t (*)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t);

// The services the turn calls through its event, in call order. Pointer arguments and results are DS offsets.
enum Service : std::uint16_t {
  NORMALIZE       = 0,  // ()
  INITIALIZE_SIDE = 1,  // ()
  DAILY           = 2,  // ()
  HALF_DAY        = 3,  // ()
  HOURLY_A        = 4,  // ()
  HOURLY_B        = 5,  // ()
  HOURLY_C        = 6,  // ()
  PRE_CONTEXT     = 7,  // ()
  FIND_SIDE       = 8,  // (side, kind) -> the side's commander of that kind
  FIND_GROUP      = 9,  // (side, division) -> the division's commander
  ACTIVE_BEGIN    = 10, // ()
  LABEL           = 11, // (format, side name, entry number, entry name)
  SIDE_SELECTED   = 12, // (side) -> the side's selected regiment
  INPUT_A         = 13, // () -> nonzero when the mouse answered
  INPUT_B         = 14, // () -> nonzero when a key was pressed
  SELECTED_BEGIN  = 15, // ()
  CONTROL_ONE     = 16, // () the player's control
  CONTROL_OTHER   = 17, // () the computer's control
  GROUP_SELECTED  = 18, // (division) -> the division's selected regiment
  GROUP_ACTION    = 19, // ()
  FINAL_PREDICATE = 20, // () -> nonzero when the regiment acts
  FINAL_ACTION    = 21, // ()
};

constexpr std::uint16_t LABEL_FORMAT{ offsetof(DataSegment, turn_label_format) };

} // namespace

void opeu_process_selected_turn(Game& game, Event event) {
  auto& data{ Data(game) };
  CallWords(game, event, NORMALIZE);
  if (!(data.side_flags[data.current_side] & Bits(SideFlag::TURN_STARTED))) {
    CallWords(game, event, INITIALIZE_SIDE);
    data.side_flags[data.current_side] |= Bits(SideFlag::TURN_STARTED);
  }
  auto& schedule{ data.schedule_flags[0] };
  if (schedule & Bits(ScheduleFlag::MONTHLY)) CallWords(game, event, DAILY);
  if (schedule & Bits(ScheduleFlag::HALF_DAY)) CallWords(game, event, HALF_DAY);
  if (schedule & Bits(ScheduleFlag::HOURLY)) {
    CallWords(game, event, HOURLY_A);
    CallWords(game, event, HOURLY_B);
    CallWords(game, event, HOURLY_C);
  }
  CallWords(game, event, PRE_CONTEXT);
  schedule                  = 0;
  data.turn_notification    = 0;
  data.context_record1      = CallWords(game, event, FIND_SIDE, data.current_side, 1);
  data.context_record2      = CallWords(game, event, FIND_SIDE, data.current_side, 2);
  data.context_record3      = CallWords(game, event, FIND_SIDE, data.current_side, 3);
  data.context_group_record = CallWords(game, event, FIND_GROUP, data.current_side, data.current_group);
  if (!(Record<Unit>(data, data.current_unit_ptr).flags & Bits(UnitFlag::READY))) return;
  if (data.side_status[data.current_side] == SIDE_PLAYED) {
    CallWords(game, event, ACTIVE_BEGIN);
    CallWords(game, event, LABEL, LABEL_FORMAT, data.side_names[data.current_side], static_cast<std::uint16_t>(data.current_group * 10 + data.current_entry + 11),
              data.entry_names[data.current_entry]);
    if (CallWords(game, event, SIDE_SELECTED, data.current_side) == data.current_unit_ptr && data.side_control[data.current_side] == SIDE_PLAYED) {
      if (data.input_mode ? CallWords(game, event, INPUT_A) : CallWords(game, event, INPUT_B)) data.side_delays[data.current_side] = 0;
    }
  }
  if (CallWords(game, event, SIDE_SELECTED, data.current_side) == data.current_unit_ptr) {
    CallWords(game, event, SELECTED_BEGIN);
    if (data.side_control[data.current_side] == SIDE_PLAYED) {
      if (data.side_delays[data.current_side] == 0) CallWords(game, event, CONTROL_ONE);
    } else
      CallWords(game, event, CONTROL_OTHER);
  }
  if (CallWords(game, event, GROUP_SELECTED, data.current_group_ptr) == data.current_unit_ptr) CallWords(game, event, GROUP_ACTION);
  if (CallWords(game, event, FINAL_PREDICATE)) CallWords(game, event, FINAL_ACTION);
}

uint16_t svc_dispatch(Game& game, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3) {
  uint16_t result{ 0 };
  switch (id) {
  case NORMALIZE:       opeu_normalize_turn_state(game); break;
  case INITIALIZE_SIDE: opeu_initialize_turn_side(game, svc_sideinit); break;
  case DAILY:           opeu_turn_daily(game, svc_daily); break;
  case HALF_DAY:        opeu_turn_half_day(game, svc_halfday); break;
  case HOURLY_A:        opeu_turn_hourly_a(game, svc_msntick); break;
  case HOURLY_B:        opeu_turn_hourly_b(game); break;
  case HOURLY_C:        opeu_turn_hourly_c(game, svc_influenc); break;
  case PRE_CONTEXT:     opeu_turn_pre_context(game); break;
  case FIND_SIDE:       result = opeu_find_side_record(game, static_cast<uint8_t>(a0), static_cast<uint8_t>(a1)); break;
  case FIND_GROUP:      result = opeu_find_group_record(game, static_cast<uint8_t>(a0), static_cast<uint8_t>(a1)); break;
  case ACTIVE_BEGIN:    opeu_turn_active_begin(game, (const DialogHost*)port_dialogs_host()); break;
  case LABEL:           {
    uint16_t const words[4]{ a0, a1, a2, a3 };
    opeu_cursor_print(game, { words, 4 });
    break;
  }
  case SIDE_SELECTED:   result = opeu_find_side_selected_unit(game, static_cast<uint8_t>(a0)); break;
  case INPUT_A:         result = static_cast<uint16_t>(opeu_turn_input_a(game, svc_mouse)); break;
  case INPUT_B:         result = opeu_turn_input_b(); break;
  case SELECTED_BEGIN:  opeu_turn_selected_begin(game); break;
  case CONTROL_ONE:     opeu_turn_control_one(game, svc_control1); break;
  case CONTROL_OTHER:   opeu_turn_control_other(game); break;
  case GROUP_SELECTED:  result = opeu_find_group_selected_unit(game, a0); break;
  case GROUP_ACTION:    opeu_turn_group_action(game, svc_grptask); break;
  case FINAL_PREDICATE: result = opeu_turn_final_predicate(game, svc_grptask); break;
  case FINAL_ACTION:    opeu_turn_final_action(game, svc_finalact); break;
  default:              port_unhandled("dispatch", id, 4);
  }
  return result;
}
