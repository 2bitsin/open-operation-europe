// The air force's dialog helpers shared by the bombing, paratroop and airlift commands: the number prompt beside the
// speaker panel, the planes free for a mission, the target-cell check and the staff confirmation.
#include "core/airhelp.hpp"
#include "common/events.hpp"
#include "core/numeric.hpp"
#include "map/grid.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/air-service.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/airbomb.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

using enum AirService;

// The texts the format service writes: the prompt line of opeu_air_ask_count, the message of opeu_air_target_valid.
constexpr std::uint16_t LINE_BYTES{ 0x18 };
constexpr std::uint16_t BUFFER_BYTES{ 0x68 };

// Message ids (message_lookup).
constexpr std::uint16_t CANNOT_TARGET{ 0x66 }; // "... %s ... %s": the mission cannot go to that cell
constexpr std::uint16_t CONFIRM_MISSION{ 0x67 };

// Map codes: a city, and the terrain no mission may target.
constexpr std::uint8_t CITY{ 1 };
} // namespace

// Shows the speaker panel with the question and "(low-high)? " (or "? " in the mouse input mode), then reads a number
// in low..high; returns it, or 0xffff when cancelled.
uint16_t opeu_air_ask_count(uint8_t ds[65536], HostEvent event, uint16_t text, uint16_t low, uint16_t high, int cancel, uint16_t person, uint16_t items) {
  ServiceLocals const locals{ LINE_BYTES };
  auto&               data{ Data(ds) };
  auto const          line{ locals.At(0) };
  Call(ds, event, SPEAKER, person, 1);
  if (data.input_mode)
    Call(ds, event, FORMAT, line, Near(data, data.air_question_mark));
  else
    Call(ds, event, FORMAT, line, Near(data, data.air_range_format), low, high);
  Call(ds, event, TEXT_FEED, text);
  Call(ds, event, TEXT_FEED, line);
  Call(ds, event, SPEAKER, person, 0);
  Call(ds, event, PRINT, Near(data, data.air_input_begin));
  auto const chosen{ Call(ds, event, VALUE, low, high, static_cast<std::uint16_t>(cancel), items) };
  Call(ds, event, PRINT, Near(data, data.air_input_end));
  return chosen;
}

// The current side's transport planes not committed to the airlift or a drop, floored at zero.
uint16_t opeu_air_free_planes(uint8_t ds[65536], HostEvent) {
  auto&         data{ Data(ds) };
  auto const&   side{ Record<Side>(data, data.current_side_ptr) };
  std::uint16_t planes{ side.planes };
  opeu_sub_word(static_cast<std::uint16_t>(side.lift.planes + side.drop.transports), &planes);
  return planes;
}

// Whether a mission (`what`) may target the cell: not a city of another side ("hostile city") and not terrain of code
// 5, 6, 8 or 10 (named by its terrain); otherwise the officer says so and 0 is returned.
uint16_t opeu_air_target_valid(uint8_t ds[65536], HostEvent event, uint8_t x, uint8_t y, uint16_t what) {
  auto&         data{ Data(ds) };
  auto const    kind{ static_cast<std::uint8_t>(opeu_map_code(ds, x, y)) };
  std::uint16_t name{};
  switch (kind) {
  case CITY:
    if (opeu_objective_side(ds, opeu_find_objective_at(ds, x, y)) == data.current_side) return 1;
    name = Near(data, data.air_hostile_city);
    break;
  case 5:
  case 6:
  case 8:
  case 10: name = data.terrain_names[kind]; break;
  default: return 1;
  }
  ServiceLocals const locals{ BUFFER_BYTES };
  auto const          buffer{ locals.At(0) };
  Call(ds, event, FORMAT, buffer, Call(ds, event, LOOKUP, CANNOT_TARGET), what, name);
  Call(ds, event, MESSAGE, buffer, data.context_record3);
  return 0;
}

// Asks the staff officer to confirm the mission.
uint16_t opeu_air_confirm(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  return Call(ds, event, CONFIRM, Call(ds, event, LOOKUP, CONFIRM_MISSION), data.context_record3);
}

uint16_t svc_airhelp(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_air(ds, "airhelp", id, args, count);
}
