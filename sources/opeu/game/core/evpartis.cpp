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
#include "common/random.hpp"
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
// opeu_hurricane_present as the far routine the daily item's presenter calls.
constexpr std::uint16_t PRESENTER_OFFSET{ 0x396 };
constexpr std::uint16_t PRESENTER_SEGMENT{ 0x108c };
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
  opeu_daily_context(game, static_cast<uint8_t>(data.current_side));
  opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(TEXT), static_cast<std::uint16_t>(opeu_message_lookup(game, static_cast<int16_t>(HURRICANE_MESSAGE))),
                                                          static_cast<std::uint16_t>(season ? data.hurricane_words[0] : data.hurricane_words[1]) });
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), TEXT, data.context_record2);
}

// In the hurricane weather: announces it, then each unheld cell of partisan terrain sees a rising at random (both
// sides are told and the cell is cleared), and each objective in use may report a rising to a watched side and loses
// 3..12 (5..14 in the season) of its garrison strength.
void opeu_partisan_event(Game& game) {
  auto& data{ Data(game) };
  if (data.weather != HURRICANE_WEATHER) return;
  bool const season{ HurricaneSeason(data) };
  auto const partisans{ season ? data.partisan_words[0] : data.partisan_words[1] };
  port_daily_show(game, HURRICANE_ITEM, data.current_side, PRESENTER_OFFSET, PRESENTER_SEGMENT);
  for (std::uint8_t y{ 0 }; y < MAP_HEIGHT; ++y)
    for (std::uint8_t x{ 0 }; x < MAP_WIDTH; ++x) {
      if (opeu_map_code(game, x, y) != PARTISAN_TERRAIN || opeu_cell_side(game, x, y) != NO_SIDE || RandomBelow(game, season ? 70 : 100) != 8) continue;
      opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(TEXT), static_cast<std::uint16_t>(opeu_message_lookup(game, static_cast<int16_t>(CELL_RISING_MESSAGE))),
                                                              static_cast<std::uint16_t>(partisans) });
      for (std::uint8_t side{ 0 }; side < 2; ++side) {
        opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), TEXT, opeu_find_side_record(game, side, COMMANDER));
        opeu_location_flash(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(side));
      }
      opeu_map_mark_5(game, x, y);
      opeu_map_erase(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
    }
  for (auto& objective : data.objectives) {
    if (!objective.active || RandomBelow(game, season ? 50 : 80)) continue;
    auto const near{ Near(data, objective) };
    if (opeu_objective_eligible(game, near)) {
      auto const side{ opeu_objective_side(game, near) };
      if (data.side_status[side] == SIDE_PLAYED) {
        opeu_request_format(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(TEXT), static_cast<std::uint16_t>(opeu_message_lookup(game, static_cast<int16_t>(OBJECTIVE_RISING_MESSAGE))),
                                                                static_cast<std::uint16_t>(partisans), static_cast<std::uint16_t>(near) });
        opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), TEXT, opeu_find_side_record(game, side, COMMANDER));
        opeu_location_flash(game, static_cast<uint8_t>(objective.x), static_cast<uint8_t>(objective.y), static_cast<uint8_t>(side));
      }
    }
    opeu_sub_byte(static_cast<std::uint16_t>(RandomBelow(game, 10) + (season ? 5 : 3)), &objective.strength);
  }
}

auto port_routine_opeu_hurricane_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_hurricane_present(game);
  return 0;
}

auto port_routine_opeu_partisan_event(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_partisan_event(game);
  return 0;
}
