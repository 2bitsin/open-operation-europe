// The commander panel of the daily display (the side's general: name, ratings, portrait) and the scenario's turn
// context.
#include "core/context.hpp"
#include "common/events.hpp"
#include "core/people.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "core/armysum.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/sound.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

extern uint16_t opeu_string_length(Game& game, uint16_t name);

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
} // namespace

// Shows the side's general in the panel unless it already shows that side (or the current side's status hides it)
// and no daily display is active; then blanks the lines below and draws the army summary.
void opeu_daily_context(Game& game, HostEvent event, uint8_t side) {
  auto& data{ Data(game) };
  if (!data.daily_active && (side == data.context_side || data.side_status[data.current_side] == SIDE_COMPUTER)) return;
  auto const  general{ opeu_find_side_record(game, side, 1) };
  auto const& person{ Record<Person>(data, general) };
  auto const  width{ std::min(static_cast<std::uint8_t>(opeu_string_length(game, general)), MAX_NAME) };
  data.context_display_word = 0;
  Call(game, event, BEGIN);
  Call(game, event, SAVE);
  Call(game, event, FRAME, 44, 24, width + 13);
  Call(game, event, PRINT, NAME_FORMAT, general);
  Call(game, event, RESTORE);
  Call(game, event, PRINT, STATS_FORMAT, person.leadership, person.infantry, person.charm, person.artillery, person.bravery, person.armor,
       (person.flags & Bits(PersonRole::POOR_HEALTH)) ? HEALTH_RED : HEALTH_NORMAL, person.health, person.experience);
  Call(game, event, PORTRAIT, 70, 40, general);
  for (std::uint16_t y{ 316 }; y < 382; y += 16) {
    Call(game, event, POSITION, 45, y);
    Call(game, event, PRINT, BLANK_FORMAT);
  }
  data.context_side = side;
  Call(game, event, FINISH);
}

void opeu_turn_pre_context(Game& game, HostEvent event) {
  auto const& data{ Data(game) };
  Call(game, event, CONTEXT, data.scenario_contexts[std::to_underlying(CurrentScenario(data))]);
}

uint16_t svc_context(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case BEGIN:    opeu_context_begin(game, (const DialogHost*)port_dialogs_host()); break;
  case SAVE:     opeu_context_save(game, (TextHost*)port_textout_host()); break;
  case FRAME:    opeu_context_frame(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case PRINT:    opeu_cursor_print(game, { args, count }); break;
  case RESTORE:  opeu_context_restore(game, (TextHost*)port_textout_host()); break;
  case PORTRAIT: opeu_context_portrait(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case POSITION: opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case FINISH:   {
    opeu_army_summary(game, svc_armysum);
    break;
  }
  case CONTEXT: opeu_set_turn_context(game, svc_sound, static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  default:      port_unhandled("context", id, count);
  }
  return 0;
}

auto port_routine_opeu_daily_context(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_daily_context(game, svc_context, static_cast<std::uint8_t>(Arg(args, count, 0)));
  return 0;
}
