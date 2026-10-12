// The arrival message: the arriving unit's name, built in a buffer on the caller's stack, in the side's format.
#include "types.hpp"
#include "core/arrshow.hpp"
#include "types/events.hpp"
#include "units/unitname.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "glue.hpp"
#include "io/soundcue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// Services leave the module through event; units/unitname issues REQUEST_FORMAT through the same callback.
enum Service : std::uint16_t {
  MESSAGE_LOOKUP  = 1, // (id) -> text
  REQUEST_FORMAT  = 2, // (destination, format, ...)
  REQUEST_MESSAGE = 3, // (text, person)
  REQUEST_WAIT    = 4,
  DAILY_CONTEXT   = 5, // (side)
};

constexpr std::uint16_t REQUEST_BUFFER{ offsetof(DataSegment, request_buffer) };
constexpr std::uint16_t ARRIVAL_FORMAT{
  75
}; // the message id of the arrival format
constexpr std::uint16_t FIRST_SIDE_TOKEN{ 54 };
constexpr std::uint16_t SECOND_SIDE_TOKEN{ 53 };
} // namespace

// scratch is the DS offset of the caller's 32-byte name buffer.
void opeu_arrival_present(uint8_t ds[65536], HostEvent event,
                          uint16_t scratch) {
  auto& data{ Data(ds) };
  Call(ds, event, REQUEST_WAIT);
  Call(ds, event, DAILY_CONTEXT, data.current_side);
  auto const name{ opeu_unit_name(ds, event, data.arrival_unit, scratch) };
  auto const side{ data.arrival_side };
  std::uint16_t const token{ side == 0 ? FIRST_SIDE_TOKEN : SECOND_SIDE_TOKEN };
  auto const          format{ Call(ds, event, MESSAGE_LOOKUP, ARRIVAL_FORMAT) };
  Call(ds, event, REQUEST_FORMAT, REQUEST_BUFFER, format, token,
       data.side_names[side], token, name);
  Call(ds, event, REQUEST_MESSAGE, REQUEST_BUFFER, data.request_person);
}

// The services of the arrival message as direct calls.
uint16_t svc_arrshow(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  uint16_t r = 0;
  switch (id) {
    case MESSAGE_LOOKUP:
      r = opeu_message_lookup(ds, svc_msglook, (int16_t)Arg(args, count, 0));
      break;
    case REQUEST_FORMAT: c1_request_format(ds, args, count); break;
    case REQUEST_MESSAGE:
      opeu_control_message(ds, (const DialogHost*)port_dialogs_host(),
                           Arg(args, count, 0), Arg(args, count, 1));
      break;
    case REQUEST_WAIT: opeu_request_wait(ds, svc_soundcue); break;
    case DAILY_CONTEXT:
      opeu_daily_context(ds, svc_context, (uint8_t)Arg(args, count, 0));
      break;
    default: port_unhandled("arrshow", id, count);
  }
  return r;
}

auto port_routine_opeu_arrival_present(std::uint8_t*        ds,
                                       std::uint16_t const* args,
                                       unsigned count) -> std::uint32_t {
  uint16_t sp;
  (void)args;
  (void)count;
  // scratch is the 32-byte name buffer below the frame pointer.
  sp = port_stack_enter(PORT_FRAME);
  opeu_arrival_present(ds, svc_arrshow, (uint16_t)(sp - 2 - 0x20));
  port_stack_leave(PORT_FRAME);
  return 0;
}
