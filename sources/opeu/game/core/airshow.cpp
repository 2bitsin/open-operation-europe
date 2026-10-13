// The air-operation display. opeu_air_sprite_mirror swaps the first two bytes of every 3-byte row of a 216-byte plane
// mask unless asked to keep it; opeu_air_sprite_draw saves the 4x32 background, draws the shadow and composes the
// plane through the side's mask; opeu_air_sprite_restore puts the background back. opeu_air_report is the weather
// check before an operation: in clear weather (0 or 1) it returns 0 (fly); otherwise the side's air commander
// reports, from the operation's remaining wait, "Bad weather has postponed the %s" (wait 6, the first delay) or
// "...has been canceled" (wait 0, result 2); any other wait waits silently (result 1).
#include "core/airshow.hpp"
#include "common/air-flight.hpp"
#include "common/events.hpp"
#include "core/people.hpp"
#include "core/template.hpp"
#include "core/textout.hpp"
#include "data/fg361c.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/dialogs.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

using namespace opeu::game;

// Far segments are the unrelocated segments plus the load segment. Locals whose addresses reach services live in the
// routine's ServiceLocals reserve.
namespace {

using PlaneMask = std::array<std::uint8_t, 216>;

constexpr std::uint8_t  PLANE_MASK{ 8 }; // the plane's entry in opeu_view_type_masks
constexpr std::uint16_t BACKGROUND_OFFSET{ 0x014a };
constexpr std::uint16_t SHADOW_OFFSET{ 0x1010 };
constexpr std::uint16_t PLANE_OFFSET{ 0x04d0 };
constexpr std::uint16_t SHADOW_MODE{ 0x20f };
constexpr std::uint16_t POSTPONED_TEXT{ offsetof(DataSegment, air_postponed_text) };
constexpr std::uint16_t CANCELED_TEXT{ offsetof(DataSegment, air_canceled_text) };
constexpr std::uint16_t REQUEST_BUFFER_SIZE{ 0x66 };
} // namespace

void opeu_air_sprite_mirror(Game& game, uint16_t image, uint8_t keep) {
  if (keep != 0) return;
  auto& mask{ Record<PlaneMask>(Data(game), image) };
  for (std::size_t i = 0; i < mask.size(); i += 3) std::swap(mask[i], mask[i + 1]);
}

void opeu_air_sprite_draw(Game& game, uint16_t load, uint16_t x, uint16_t y, uint8_t keep) {
  ServiceLocals const locals{ sizeof(PlaneMask) }; // the mask the compose service reads
  auto&               data{ Data(game) };
  auto const          mask{ locals.At(0) };
  std::ranges::copy(opeu_view_type_masks[PLANE_MASK], Record<PlaneMask>(data, mask).begin());
  opeu_air_sprite_mirror(game, mask, keep);
  SetStream(data, static_cast<std::uint16_t>(load + BACKGROUND_SEGMENT), BACKGROUND_OFFSET);
  opeu_bitmap_save(x, y, 4, 32, port_far(data.stream_ptr[1], data.stream_ptr[0]));
  SetStream(data, static_cast<std::uint16_t>(load + SPRITE_SEGMENT), SHADOW_OFFSET);
  opeu_draw_far(x + 1, y + 8, 3, 24, SHADOW_MODE, port_far(data.stream_ptr[1], data.stream_ptr[0]));
  SetStream(data, static_cast<std::uint16_t>(load + SPRITE_SEGMENT), PLANE_OFFSET);
  opeu_sprite_compose(x, y, 3, 24, port_far(data.stream_ptr[1], data.stream_ptr[0]), port_far(load + DATA_SEGMENT, mask), port_far(data.bitmap_buffer, 0), 0);
}

void opeu_air_sprite_restore(Game& game, uint16_t load, uint16_t x, uint16_t y) {
  auto& data{ Data(game) };
  SetStream(data, static_cast<std::uint16_t>(load + BACKGROUND_SEGMENT), BACKGROUND_OFFSET);
  opeu_bitmap_restore(x, y, 4, 32, 0, port_far(data.stream_ptr[1], data.stream_ptr[0]));
}

uint8_t opeu_air_report(Game& game, uint16_t what, uint8_t wait) {
  ServiceLocals const locals{ REQUEST_BUFFER_SIZE };
  auto&               data{ Data(game) };
  std::uint16_t const buffer{ locals.At(0) };
  auto const&         commander{ Record<Person>(data, opeu_find_side_record(game, data.air_side, FLIGHT_COMMANDER_KIND)) };
  std::uint16_t const person{ opeu_find_person_template(game, commander.status, AIR_OFFICER_TEMPLATE) };
  if (data.weather != 0 && data.weather != 1) {
    if (wait != 0 && wait != 6) return 1;
    std::uint16_t const format{ wait ? POSTPONED_TEXT : CANCELED_TEXT };
    opeu_request_format(game, std::array<std::uint16_t, 4>{ buffer, format, static_cast<std::uint16_t>(SideColorCode(data.air_side)), what });
    opeu_control_message(game, buffer, person);
    return wait ? 1 : 2;
  }
  return 0;
}
