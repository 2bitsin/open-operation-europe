// The hurricane and partisan events: in the hurricane weather, partisans may rise in unheld cells of partisan terrain
// and at objectives, which lose garrison strength. In the Normandy scenario from July to September the hurricane
// season raises the odds and picks the season's texts.
#include "core/evpartis.hpp"
#include "common/events.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "io/soundcue.hpp"
#include "link/glue.hpp"
#include "map/grid.hpp"
#include "map/maperase.hpp"
#include "map/mapunit.hpp"
#include "map/objeleg.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t TEXT{ offsetof(DataSegment, event_buffer) };
constexpr std::uint8_t  HURRICANE_WEATHER{ 5 };
constexpr std::uint16_t HURRICANE_ITEM{ 0xe7 }; // the daily item that announces the hurricane
constexpr std::uint16_t HURRICANE_MESSAGE{ 0x6d };
constexpr std::uint16_t CELL_RISING_MESSAGE{ 0x1b };
constexpr std::uint16_t OBJECTIVE_RISING_MESSAGE{ 0x1c };
constexpr std::uint8_t  PARTISAN_TERRAIN{ 10 };
constexpr std::uint8_t  COMMANDER{ 3 }; // opeu_find_side_record's kind for the side's commander

// The hurricane season: Normandy from July to September.
auto HurricaneSeason(DataSegment const& data) -> bool {
  return CurrentScenario(data) == Scenario::D_DAY && data.month >= 7 && data.month <= 9;
}
} // namespace

// The hurricane's announcement: cue, the current side's panel and the message from the panel's general.
void opeu_hurricane_present(Game& game) {
  auto const& data{ Data(game) };
  bool const  season{ HurricaneSeason(data) };
  opeu_sound_cue15(game);
  opeu_daily_context(game, data.current_side);
  opeu_request_format(game, std::array<std::uint16_t, 3>{ TEXT, opeu_message_lookup(game, HURRICANE_MESSAGE), (season ? data.hurricane_words[0] : data.hurricane_words[1]) });
  opeu_control_message(game, TEXT, data.context_record2);
}

// In the hurricane weather: announces it, then each unheld cell of partisan terrain sees a rising at random (both
// sides are told and the cell is cleared), and each objective in use may report a rising to a watched side and loses
// 3..12 (5..14 in the season) of its garrison strength.
void opeu_partisan_event(Game& game) {
  auto& data{ Data(game) };
  if (data.weather != HURRICANE_WEATHER) return;
  bool const season{ HurricaneSeason(data) };
  auto const partisans{ season ? data.partisan_words[0] : data.partisan_words[1] };
  opeu_daily_show(game, HURRICANE_ITEM, data.current_side, Presenter::HURRICANE);
  for (std::uint8_t y{ 0 }; y < MAP_HEIGHT; ++y)
    for (std::uint8_t x{ 0 }; x < MAP_WIDTH; ++x) {
      if (opeu_map_code(game, x, y) != PARTISAN_TERRAIN || opeu_cell_side(game, x, y) != NO_SIDE || game.State().RandomBounded(season ? 70 : 100) != 8) continue;
      opeu_request_format(game, std::array<std::uint16_t, 3>{ TEXT, opeu_message_lookup(game, CELL_RISING_MESSAGE), partisans });
      for (std::uint8_t side{ 0 }; side < 2; ++side) {
        opeu_control_message(game, TEXT, opeu_find_side_record(game, side, COMMANDER));
        opeu_location_flash(game, x, y, side);
      }
      opeu_map_mark_5(game, x, y);
      opeu_map_erase(game, x, y);
    }
  for (auto& objective : data.objectives) {
    if (!objective.active || game.State().RandomBounded(season ? 50 : 80)) continue;
    auto const near{ Near(data, objective) };
    if (opeu_objective_eligible(game, near)) {
      auto const side{ opeu_objective_side(game, near) };
      if (data.side_status[side] == SIDE_PLAYED) {
        opeu_request_format(game, std::array<std::uint16_t, 4>{ TEXT, opeu_message_lookup(game, OBJECTIVE_RISING_MESSAGE), partisans, near });
        opeu_control_message(game, TEXT, opeu_find_side_record(game, side, COMMANDER));
        opeu_location_flash(game, objective.x, objective.y, side);
      }
    }
    opeu_sub_byte(game.State().RandomBounded(10) + (season ? 5 : 3), &objective.strength);
  }
}
