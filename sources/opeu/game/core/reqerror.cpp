// The message that tells the player why a filed request could not be carried out.
#include "core/reqerror.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "glue.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// The services opeu_request_error_present calls through its event.
enum class ErrorService : uint16_t {
  MESSAGE_TEXT = 1, // opeu_message_lookup(id) -> near pointer of the text
  FORMAT       = 2, // request_format(destination, format, words...)
  SHOW         = 3, // opeu_control_message(text, person)
  PREPARE      = 4, // opeu_pending_prepare
  FINISH       = 5, // opeu_pending_finish(value)
};

// The first of the three reasons' messages: the objective is lost, no free place, no room for the delivery.
constexpr uint16_t FIRST_REASON_MESSAGE{ 61 };

} // namespace

// Formats the reason the request (invalid_request) failed into the request buffer and shows it from the side's chief
// of staff. When no side is played by a person the message closes by itself.
void opeu_request_error_present(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  Call(ds, event, ErrorService::PREPARE);
  auto const& request{ Record<RequestSlot>(data, data.invalid_request) };
  auto const  buffer{ static_cast<uint16_t>(offsetof(DataSegment, request_buffer)) };
  auto const  name{ data.request_kind_names[request.kind] };
  auto const  format{ Call(ds, event, ErrorService::MESSAGE_TEXT, FIRST_REASON_MESSAGE + data.request_error) };
  if (data.request_error == 0)
    Call(ds, event, ErrorService::FORMAT, buffer, format, request.target, name);
  else
    Call(ds, event, ErrorService::FORMAT, buffer, format, name);
  Call(ds, event, ErrorService::SHOW, buffer, data.request_template);
  if (data.side_control[1] == SIDE_COMPUTER && data.side_control[0] == SIDE_COMPUTER) Call(ds, event, ErrorService::FINISH, 3);
}

uint16_t svc_reqerror(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (static_cast<ErrorService>(id)) {
  case ErrorService::MESSAGE_TEXT: r = opeu_message_lookup(ds, svc_msglook, (short)Arg(args, count, 0)); break;
  case ErrorService::FORMAT:       opeu_request_format(ds, { args, count }); break;
  case ErrorService::SHOW:         opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case ErrorService::PREPARE:      opeu_pending_prepare(ds, svc_soundcue); break;
  case ErrorService::FINISH:       opeu_pending_finish(ds, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  default:                         port_unhandled("reqerror", id, count);
  }
  return r;
}

auto port_routine_opeu_request_error_present(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_request_error_present(ds, svc_reqerror);
  return 0;
}
