// The arrival message: the arriving unit's name, built in a buffer on the caller's stack, in the side's format.
#include "core/arrshow.hpp"
#include "common/events.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/unitname.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "io/soundcue.hpp"
#include "link/glue.hpp"
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
constexpr std::uint16_t ARRIVAL_FORMAT{ 75 }; // the message id of the arrival format
constexpr std::uint16_t FIRST_SIDE_TOKEN{ 54 };
constexpr std::uint16_t SECOND_SIDE_TOKEN{ 53 };
} // namespace

// The arriving unit's name goes into a 32-byte buffer of the routine's reserve for the format service.
void opeu_arrival_present(Game& game, HostEvent event) {
  ServiceLocals const locals{ 0x20 };
  auto&               data{ Data(game) };
  Call(game, event, REQUEST_WAIT);
  Call(game, event, DAILY_CONTEXT, data.current_side);
  auto const          name{ opeu_unit_name(game, event, data.arrival_unit, locals.At(0)) };
  auto const          side{ data.arrival_side };
  std::uint16_t const token{ side == 0 ? FIRST_SIDE_TOKEN : SECOND_SIDE_TOKEN };
  auto const          format{ Call(game, event, MESSAGE_LOOKUP, ARRIVAL_FORMAT) };
  Call(game, event, REQUEST_FORMAT, REQUEST_BUFFER, format, token, data.side_names[side], token, name);
  Call(game, event, REQUEST_MESSAGE, REQUEST_BUFFER, data.request_person);
}

// The services of the arrival message as direct calls.
uint16_t svc_arrshow(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case MESSAGE_LOOKUP:  r = opeu_message_lookup(game, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  case REQUEST_FORMAT:  opeu_request_format(game, { args, count }); break;
  case REQUEST_MESSAGE: opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case REQUEST_WAIT:    opeu_request_wait(game, svc_soundcue); break;
  case DAILY_CONTEXT:   opeu_daily_context(game, svc_context, (uint8_t)Arg(args, count, 0)); break;
  default:              port_unhandled("arrshow", id, count);
  }
  return r;
}

auto port_routine_opeu_arrival_present(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_arrival_present(game, svc_arrshow);
  return 0;
}
