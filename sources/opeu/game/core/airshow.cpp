// The air-operation display. opeu_air_sprite_mirror swaps the first two bytes of every 3-byte row of a 216-byte plane
// mask unless asked to keep it; opeu_air_sprite_draw saves the 4x32 background, draws the shadow and composes the
// plane through the side's mask; opeu_air_sprite_restore puts the background back. opeu_air_report is the weather
// check before an operation: in clear weather (0 or 1) it returns 0 (fly); otherwise the side's air commander
// reports, from the operation's remaining wait, "Bad weather has postponed the %s" (wait 6, the first delay) or
// "...has been canceled" (wait 0, result 2); any other wait waits silently (result 1).
#include "core/airshow.hpp"
#include "common/events.hpp"
#include "core/people.hpp"
#include "core/template.hpp"
#include "core/textout.hpp"
#include "data/fg361c.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

using namespace opeu::game;

// Far segments are the unrelocated segments plus the load segment. Locals whose addresses reach services live at
// their frame offsets below bp (the routine's own frame pointer).
namespace {

// Services leave the module through event.
enum Service : std::uint16_t {
  BITMAP_SAVE     = 0, // (x, y, w, h, segment, offset)
  DRAW_FAR        = 1, // (x, y, w, h, mode, segment, offset)
  SPRITE_COMPOSE  = 2, // (x, y, w, h, image offset, image segment, mask offset, mask segment, mode, segment, offset)
  BITMAP_RESTORE  = 3, // (x, y, w, h, mode, segment, offset)
  REQUEST_FORMAT  = 4, // (destination, format, colour, argument)
  CONTROL_MESSAGE = 5, // (text, person)
};

using PlaneMask = std::array<std::uint8_t, 216>;

constexpr std::uint8_t  PLANE_MASK{ 8 }; // the plane's entry in opeu_view_type_masks
constexpr std::uint16_t BACKGROUND_OFFSET{ 0x014a };
constexpr std::uint16_t SHADOW_OFFSET{ 0x1010 };
constexpr std::uint16_t PLANE_OFFSET{ 0x04d0 };
constexpr std::uint16_t SHADOW_MODE{ 0x20f };
constexpr std::uint16_t POSTPONED_TEXT{ offsetof(DataSegment, air_postponed_text) };
constexpr std::uint16_t CANCELED_TEXT{ offsetof(DataSegment, air_canceled_text) };
constexpr std::uint16_t REQUEST_BUFFER_SIZE{ 0x66 };
constexpr std::uint8_t  RANK_AIR{ 1 };
constexpr std::uint8_t  TEMPLATE_AIR{ 6 };

// Points the graphics stream at segment:offset.
auto SetStream(DataSegment& data, std::uint16_t segment, std::uint16_t offset) -> void {
  data.stream_ptr[0] = offset;
  data.stream_ptr[1] = segment;
}
} // namespace

void opeu_air_sprite_mirror(uint8_t ds[65536], uint16_t image, uint8_t keep) {
  if (keep != 0) return;
  auto& mask{ Record<PlaneMask>(Data(ds), image) };
  for (std::size_t i = 0; i < mask.size(); i += 3) std::swap(mask[i], mask[i + 1]);
}

void opeu_air_sprite_draw(uint8_t ds[65536], HostEvent event, uint16_t load, uint16_t bp, uint16_t x, uint16_t y, uint8_t keep) {
  auto&         data{ Data(ds) };
  std::uint16_t mask{ Local(bp, 0xdc) };
  std::ranges::copy(opeu_view_type_masks[PLANE_MASK], Record<PlaneMask>(data, mask).begin());
  Record<std::uint8_t>(data, Local(bp, 0xde)) = keep;
  opeu_air_sprite_mirror(ds, mask, keep);
  SetStream(data, static_cast<std::uint16_t>(load + BACKGROUND_SEGMENT), BACKGROUND_OFFSET);
  Call(ds, event, BITMAP_SAVE, x, y, 4, 32, data.stream_ptr[1], data.stream_ptr[0]);
  SetStream(data, static_cast<std::uint16_t>(load + SPRITE_SEGMENT), SHADOW_OFFSET);
  Call(ds, event, DRAW_FAR, x + 1, y + 8, 3, 24, SHADOW_MODE, data.stream_ptr[1], data.stream_ptr[0]);
  SetStream(data, static_cast<std::uint16_t>(load + SPRITE_SEGMENT), PLANE_OFFSET);
  SetWord(ds, Local(bp, 4), mask);
  Call(ds, event, SPRITE_COMPOSE, x, y, 3, 24, data.stream_ptr[0], data.stream_ptr[1], Word(ds, Local(bp, 4)), load + DATA_SEGMENT, 0, data.bitmap_buffer, 0);
}

void opeu_air_sprite_restore(uint8_t ds[65536], HostEvent event, uint16_t load, uint16_t x, uint16_t y) {
  auto& data{ Data(ds) };
  SetStream(data, static_cast<std::uint16_t>(load + BACKGROUND_SEGMENT), BACKGROUND_OFFSET);
  Call(ds, event, BITMAP_RESTORE, x, y, 4, 32, 0, data.stream_ptr[1], data.stream_ptr[0]);
}

uint8_t opeu_air_report(uint8_t ds[65536], HostEvent event, uint16_t bp, uint16_t what, uint8_t wait) {
  auto&               data{ Data(ds) };
  std::uint16_t const buffer{ Local(bp, REQUEST_BUFFER_SIZE) };
  auto const&         commander{ Record<Person>(data, opeu_find_side_record(ds, data.air_side, RANK_AIR)) };
  std::uint16_t const person{ opeu_find_person_template(ds, commander.status, TEMPLATE_AIR) };
  if (data.weather != 0 && data.weather != 1) {
    if (wait != 0 && wait != 6) return 1;
    std::uint16_t const format{ wait ? POSTPONED_TEXT : CANCELED_TEXT };
    Call(ds, event, REQUEST_FORMAT, buffer, format, data.air_side == 0 ? '6' : '5', what);
    Call(ds, event, CONTROL_MESSAGE, buffer, person);
    return wait ? 1 : 2;
  }
  return 0;
}

// The services of the air-operation display as direct calls; far (segment, offset) pairs become host pointers.
uint16_t svc_airshow(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case BITMAP_SAVE: opeu_bitmap_save(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), port_far(Arg(args, count, 4), Arg(args, count, 5))); break;
  case DRAW_FAR:
    r = (uint16_t)opeu_draw_far(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), port_far(Arg(args, count, 5), Arg(args, count, 6)));
    break;
  case SPRITE_COMPOSE:
    opeu_sprite_compose(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), port_far(Arg(args, count, 5), Arg(args, count, 4)),
                        port_far(Arg(args, count, 7), Arg(args, count, 6)), port_far(Arg(args, count, 9), Arg(args, count, 10)), Arg(args, count, 8));
    break;
  case BITMAP_RESTORE:
    r = (uint16_t)opeu_bitmap_restore(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), port_far(Arg(args, count, 5), Arg(args, count, 6)));
    break;
  case REQUEST_FORMAT:  opeu_request_format(ds, { args, count }); break;
  case CONTROL_MESSAGE: opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  default:              port_unhandled("airshow", id, count);
  }
  return r;
}
