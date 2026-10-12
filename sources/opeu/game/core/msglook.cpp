// Looking up a battle message: the far text of the message id is copied into the shared message buffer after the
// current side's daily context is selected.
#include "core/msglook.hpp"
#include "common/events.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include "core/context.hpp"
#include "core/memory.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// The services opeu_message_lookup calls through its event.
enum MessageService : uint16_t {
  SELECT_CONTEXT = 0, // daily_context(side)
  COPY_TEXT      = 1, // string_copy_far(destination, offset, segment)
  REPORT_ERROR   = 2, // request_format(destination, text)
};

} // namespace

// Returns the near pointer of the message buffer. Every id is looked up: the game's range test compares the truth
// value of 0 < id with 135 and so always passes, and the dispatch error is never reported.
uint16_t opeu_message_lookup(uint8_t ds[65536], HostEvent event, int16_t id) {
  auto const&    data{ Data(ds) };
  uint16_t const side[]{ data.current_side };
  event(ds, SELECT_CONTEXT, side, 1);
  auto const&    text{ data.message_table[static_cast<uint16_t>(id)] };
  uint16_t const copy[]{ offsetof(DataSegment, message_buffer), text.offset, text.segment };
  event(ds, COPY_TEXT, copy, 3);
  return offsetof(DataSegment, message_buffer);
}

uint16_t svc_msglook(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case SELECT_CONTEXT: opeu_daily_context(ds, svc_context, (uint8_t)Arg(args, count, 0)); break;
  case COPY_TEXT:
    r = opeu_string_copy_far(ds, Arg(args, count, 0), port_far(Arg(args, count, 2), 0), Arg(args, count, 1));
    break;
  case REPORT_ERROR: b1_request_format(ds, args, count); break;
  default:           port_unhandled("msglook", id, count);
  }
  return r;
}
