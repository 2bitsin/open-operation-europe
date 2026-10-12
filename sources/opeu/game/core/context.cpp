// The commander panel of the daily display (the side's general: name, ratings, portrait) and the scenario's turn
// context.
#include "core/context.hpp"
#include "common/events.hpp"
#include "core/people.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "core/armysum.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "io/sound.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

extern uint16_t opeu_string_length(uint8_t ds[65536], uint16_t name);

using namespace opeu::game;

namespace {

// The services the panel calls through its event.
enum Service : std::uint16_t {
  BEGIN    = 0, // ()
  SAVE     = 1, // () the text state
  FRAME    = 2, // (x, y, width)
  PRINT    = 3, // (format, words...)
  RESTORE  = 4, // () the text state
  PORTRAIT = 5, // (x, y, person)
  POSITION = 6, // (x, y)
  FINISH   = 7, // () the army summary
  CONTEXT  = 8, // (context)
};

constexpr std::uint16_t NAME_FORMAT{ offsetof(DataSegment, context_name_format) };
constexpr std::uint16_t STATS_FORMAT{ offsetof(DataSegment, context_stats_format) };
constexpr std::uint16_t BLANK_FORMAT{ offsetof(DataSegment, context_blank_format) };
constexpr std::uint8_t  MAX_NAME{ 13 };
constexpr std::uint16_t HEALTH_RED{ 50 };    // the text color of a poor health rating
constexpr std::uint16_t HEALTH_NORMAL{ 54 }; // and of any other
constexpr std::uint16_t ARMY_WORKSPACE{ 82 };
} // namespace

// Shows the side's general in the panel unless it already shows that side (or the current side's status hides it)
// and no daily display is active; then blanks the lines below and draws the army summary.
void opeu_daily_context(uint8_t ds[65536], HostEvent event, uint8_t side) {
  auto& data{ Data(ds) };
  if (!data.daily_active && (side == data.context_side || data.side_status[data.current_side] == 0)) return;
  auto const  general{ opeu_find_side_record(ds, side, 1) };
  auto const& person{ Record<Person>(data, general) };
  auto const  width{ std::min(static_cast<std::uint8_t>(opeu_string_length(ds, general)), MAX_NAME) };
  data.context_display_word = 0;
  Call(ds, event, BEGIN);
  Call(ds, event, SAVE);
  Call(ds, event, FRAME, 44, 24, width + 13);
  Call(ds, event, PRINT, NAME_FORMAT, general);
  Call(ds, event, RESTORE);
  Call(ds, event, PRINT, STATS_FORMAT, person.leadership, person.infantry, person.charm, person.artillery, person.bravery, person.armor,
       (person.flags & Bits(PersonRole::POOR_HEALTH)) ? HEALTH_RED : HEALTH_NORMAL, person.health, person.experience);
  Call(ds, event, PORTRAIT, 70, 40, general);
  for (std::uint16_t y{ 316 }; y < 382; y += 16) {
    Call(ds, event, POSITION, 45, y);
    Call(ds, event, PRINT, BLANK_FORMAT);
  }
  data.context_side = side;
  Call(ds, event, FINISH);
}

void opeu_turn_pre_context(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  Call(ds, event, CONTEXT, data.scenario_contexts[std::to_underlying(CurrentScenario(data))]);
}

uint16_t svc_context(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case BEGIN:    opeu_context_begin(ds, (const DialogHost*)port_dialogs_host()); break;
  case SAVE:     opeu_context_save(ds, (TextHost*)port_textout_host()); break;
  case FRAME:    opeu_context_frame(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case PRINT:    b1_cursor_print(ds, args, count); break;
  case RESTORE:  opeu_context_restore(ds, (TextHost*)port_textout_host()); break;
  case PORTRAIT: opeu_context_portrait(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case POSITION: opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case FINISH:   {
    auto const workspace{ b1_stack_alloc(ARMY_WORKSPACE) };
    opeu_army_summary(ds, svc_armysum, static_cast<std::uint16_t>(workspace + 2));
    port_scratch_leave(ARMY_WORKSPACE);
    break;
  }
  case CONTEXT: opeu_set_turn_context(ds, svc_sound, static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  default:      port_unhandled("context", id, count);
  }
  return 0;
}

auto port_routine_opeu_daily_context(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_daily_context(ds, svc_context, static_cast<std::uint8_t>(Arg(args, count, 0)));
  return 0;
}
