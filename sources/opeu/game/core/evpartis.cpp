// The hurricane and partisan events: in the hurricane weather, partisans may rise in unheld cells of partisan terrain
// and at objectives, which lose garrison strength. In the Normandy scenario from July to September the hurricane
// season raises the odds and picks the season's texts.
#include "types.hpp"
#include "core/evpartis.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "common/events.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "common/scenario.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "glue.hpp"
#include "io/soundcue.hpp"
#include "map/grid.hpp"
#include "map/maperase.hpp"
#include "map/mapunit.hpp"
#include "map/objeleg.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the events call through their event.
enum Service : std::uint16_t {
  CUE        = 0, // () sound cue 15
  CONTEXT    = 1, // (side) the commander panel
  LOOKUP     = 2, // (message) -> its text
  FORMAT     = 3, // (destination, format, words...)
  MESSAGE    = 4, // (text, person)
  DAILY_SHOW = 5, // (daily item, side, presenter offset, presenter segment)
  RISING     = 6, // (x, y, side) flash the cell on the side's map
  ERASE      = 7, // (x, y) redraw the cell
};

constexpr std::uint16_t TEXT{ offsetof(DataSegment, event_buffer) };
constexpr std::uint8_t  HURRICANE_WEATHER{ 5 };
constexpr std::uint16_t HURRICANE_ITEM{
  0xe7
}; // the daily item that announces the hurricane
constexpr std::uint16_t HURRICANE_MESSAGE{ 0x6d };
constexpr std::uint16_t CELL_RISING_MESSAGE{ 0x1b };
constexpr std::uint16_t OBJECTIVE_RISING_MESSAGE{ 0x1c };
// opeu_hurricane_present as the far routine the daily item's presenter calls.
constexpr std::uint16_t PRESENTER_OFFSET{ 0x396 };
constexpr std::uint16_t PRESENTER_SEGMENT{ 0x108c };
constexpr std::uint8_t  PARTISAN_TERRAIN{ 10 };
constexpr std::uint8_t  NO_SIDE{ 0xff };
constexpr std::uint8_t  MAP_SIZE{ 40 };
constexpr std::uint8_t  COMMANDER{
  3
}; // opeu_find_side_record's kind for the side's commander

// The hurricane season: Normandy from July to September.
auto HurricaneSeason(DataSegment const& data) -> bool {
  return CurrentScenario(data) == Scenario::D_DAY && data.month >= 7 &&
         data.month <= 9;
}
} // namespace

// The hurricane's announcement: cue, the current side's panel and the message from the panel's general.
void opeu_hurricane_present(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  bool const  season{ HurricaneSeason(data) };
  Call(ds, event, CUE);
  Call(ds, event, CONTEXT, data.current_side);
  Call(ds, event, FORMAT, TEXT, Call(ds, event, LOOKUP, HURRICANE_MESSAGE),
       season ? data.hurricane_words[0] : data.hurricane_words[1]);
  Call(ds, event, MESSAGE, TEXT, data.context_record2);
}

// In the hurricane weather: announces it, then each unheld cell of partisan terrain sees a rising at random (both
// sides are told and the cell is cleared), and each objective in use may report a rising to a watched side and loses
// 3..12 (5..14 in the season) of its garrison strength.
void opeu_partisan_event(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  if (data.weather != HURRICANE_WEATHER)
    return;
  bool const season{ HurricaneSeason(data) };
  auto const partisans{ season ? data.partisan_words[0]
                               : data.partisan_words[1] };
  Call(ds, event, DAILY_SHOW, HURRICANE_ITEM, data.current_side,
       PRESENTER_OFFSET, PRESENTER_SEGMENT);
  for (std::uint8_t y{ 0 }; y < MAP_SIZE; ++y)
    for (std::uint8_t x{ 0 }; x < MAP_SIZE; ++x) {
      if (opeu_map_code(ds, x, y) != PARTISAN_TERRAIN ||
          opeu_cell_side(ds, x, y) != NO_SIDE ||
          RandomBelow(ds, season ? 70 : 100) != 8)
        continue;
      Call(ds, event, FORMAT, TEXT,
           Call(ds, event, LOOKUP, CELL_RISING_MESSAGE), partisans);
      for (std::uint8_t side{ 0 }; side < 2; ++side) {
        Call(ds, event, MESSAGE, TEXT,
             opeu_find_side_record(ds, side, COMMANDER));
        Call(ds, event, RISING, x, y, side);
      }
      opeu_map_mark_5(ds, x, y);
      Call(ds, event, ERASE, x, y);
    }
  for (auto& objective : data.objectives) {
    if (!objective.active || RandomBelow(ds, season ? 50 : 80))
      continue;
    auto const near{ Near(data, objective) };
    if (opeu_objective_eligible(ds, near)) {
      auto const side{ opeu_objective_side(ds, near) };
      if (data.side_status[side] == SIDE_PLAYED) {
        Call(ds, event, FORMAT, TEXT,
             Call(ds, event, LOOKUP, OBJECTIVE_RISING_MESSAGE), partisans,
             near);
        Call(ds, event, MESSAGE, TEXT,
             opeu_find_side_record(ds, side, COMMANDER));
        Call(ds, event, RISING, objective.x, objective.y, side);
      }
    }
    opeu_sub_byte(
        static_cast<std::uint16_t>(RandomBelow(ds, 10) + (season ? 5 : 3)),
        &objective.strength);
  }
}

uint16_t svc_evpartis(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
    case CUE: opeu_sound_cue15(ds, svc_soundcue); break;
    case CONTEXT:
      opeu_daily_context(ds, svc_context,
                         static_cast<uint8_t>(Arg(args, count, 0)));
      break;
    case LOOKUP:
      result = opeu_message_lookup(ds, svc_msglook,
                                   static_cast<int16_t>(Arg(args, count, 0)));
      break;
    case FORMAT: c1_request_format(ds, args, count); break;
    case MESSAGE:
      opeu_control_message(ds, (const DialogHost*)port_dialogs_host(),
                           Arg(args, count, 0), Arg(args, count, 1));
      break;
    case DAILY_SHOW:
      port_daily_show(ds, Arg(args, count, 0), Arg(args, count, 1),
                      Arg(args, count, 2), Arg(args, count, 3));
      break;
    case RISING: {
      auto const frame{ port_frame_enter() };
      opeu_location_flash(ds, svc_mapunit, frame,
                          static_cast<uint8_t>(Arg(args, count, 0)),
                          static_cast<uint8_t>(Arg(args, count, 1)),
                          static_cast<uint8_t>(Arg(args, count, 2)));
      port_frame_leave();
      break;
    }
    case ERASE: {
      auto const frame{ port_frame_enter() };
      opeu_map_erase(ds, svc_maperase, frame,
                     static_cast<uint8_t>(Arg(args, count, 0)),
                     static_cast<uint8_t>(Arg(args, count, 1)));
      port_frame_leave();
      break;
    }
    default: port_unhandled("evpartis", id, count);
  }
  return result;
}

auto port_routine_opeu_hurricane_present(std::uint8_t* ds, std::uint16_t const*,
                                         unsigned) -> std::uint32_t {
  opeu_hurricane_present(ds, svc_evpartis);
  return 0;
}

auto port_routine_opeu_partisan_event(std::uint8_t* ds, std::uint16_t const*,
                                      unsigned) -> std::uint32_t {
  opeu_partisan_event(ds, svc_evpartis);
  return 0;
}
