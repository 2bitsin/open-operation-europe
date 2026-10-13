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
#include <array>
#include <cstddef>
#include <utility>

extern uint16_t opeu_string_length(Game& game, uint16_t name);

using namespace opeu::game;

namespace {

constexpr std::uint16_t NAME_FORMAT{ offsetof(DataSegment, context_name_format) };
constexpr std::uint16_t STATS_FORMAT{ offsetof(DataSegment, context_stats_format) };
constexpr std::uint16_t BLANK_FORMAT{ offsetof(DataSegment, context_blank_format) };
constexpr std::uint8_t  MAX_NAME{ 13 };
constexpr std::uint16_t HEALTH_RED{ 50 };    // the text color of a poor health rating
constexpr std::uint16_t HEALTH_NORMAL{ 54 }; // and of any other
} // namespace

// Shows the side's general in the panel unless it already shows that side (or the current side's status hides it)
// and no daily display is active; then blanks the lines below and draws the army summary.
void opeu_daily_context(Game& game, uint8_t side) {
  auto& data{ Data(game) };
  if (!data.daily_active && (side == data.context_side || data.side_status[data.current_side] == SIDE_COMPUTER)) return;
  auto const  general{ opeu_find_side_record(game, side, 1) };
  auto const& person{ Record<Person>(data, general) };
  auto const  width{ std::min(static_cast<std::uint8_t>(opeu_string_length(game, general)), MAX_NAME) };
  data.context_display_word = 0;
  opeu_context_begin(game);
  opeu_context_save(game);
  opeu_context_frame(game, 44, 24, width + 13);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ NAME_FORMAT, general });
  opeu_context_restore(game);
  opeu_cursor_print(game, std::array<std::uint16_t, 10>{ STATS_FORMAT, static_cast<std::uint16_t>(person.leadership), static_cast<std::uint16_t>(person.infantry),
                                                         static_cast<std::uint16_t>(person.charm), static_cast<std::uint16_t>(person.artillery), static_cast<std::uint16_t>(person.bravery),
                                                         static_cast<std::uint16_t>(person.armor), ((person.flags & Bits(PersonRole::POOR_HEALTH)) ? HEALTH_RED : HEALTH_NORMAL),
                                                         static_cast<std::uint16_t>(person.health), static_cast<std::uint16_t>(person.experience) });
  opeu_context_portrait(game, 70, 40, general);
  for (std::uint16_t y{ 316 }; y < 382; y += 16) {
    opeu_army_position(game, 45, y);
    opeu_cursor_print(game, std::array<std::uint16_t, 1>{ BLANK_FORMAT });
  }
  data.context_side = side;
  opeu_army_summary(game);
}

void opeu_turn_pre_context(Game& game) {
  auto const& data{ Data(game) };
  opeu_set_turn_context(game, data.scenario_contexts[std::to_underlying(CurrentScenario(data))]);
}
