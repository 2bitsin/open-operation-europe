// The request menu's shared pieces: its service table and the helpers its choices use.
#include "core/rqcommon.hpp"
#include "common/memory.hpp"
#include "data-segment.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "glue.hpp"
#include "io/waitkey.hpp"
#include "map/mappick.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {

constexpr uint16_t TEXT_BYTES{ 0x7c }; // the prompt text the format service writes
// The far address of opeu_request_finish, which SHOW calls back.
constexpr uint16_t FINISH_PRESENT_OFFSET{ 0x009a };
constexpr uint16_t FINISH_PRESENT_SEGMENT{ 0x22f9 };
// The daily dialog of the request reports.
constexpr uint16_t REQUEST_SHOW{ 0x73 };

auto FiledRequests(DataSegment& data) -> auto& {
  return Record<Side>(data, data.current_side_ptr).requests;
}
auto FiledRequests(DataSegment const& data) -> auto const& {
  return Record<Side>(data, data.current_side_ptr).requests;
}

} // namespace

// "You cannot increase your %s any more.", spoken by the context's third person.
void opeu_request_no_more(uint8_t ds[65536], HostEvent event, uint16_t what) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(ds) };
  auto const          buffer{ locals.At(0) };
  Call(ds, event, RequestMenuService::FORMAT, buffer, offsetof(DataSegment, rq_no_more_text), what);
  Call(ds, event, RequestMenuService::MESSAGE, buffer, data.context_record3);
}

// Whether the current side has filed a request of this kind (supplies, battalion or regiment).
uint16_t opeu_request_pending(const uint8_t ds[65536], uint8_t kind) {
  return FiledRequests(Data(ds))[kind].kind != std::to_underlying(RequestKind::NONE) ? 1 : 0;
}

// Whether the current side's booked request is of this kind.
uint16_t opeu_request_mode_is(const uint8_t ds[65536], uint8_t mode) {
  auto const& data{ Data(ds) };
  return data.request_configs[data.current_side].kind == mode ? 1 : 0;
}

// "Your %s request has been sent. Cancel the request?"; returns the context's second person's answer.
uint16_t opeu_request_cancel_prompt(uint8_t ds[65536], HostEvent event, uint16_t what) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(ds) };
  auto const          buffer{ locals.At(0) };
  Call(ds, event, RequestMenuService::FORMAT, buffer, offsetof(DataSegment, rq_cancel_text), what);
  return Call(ds, event, RequestMenuService::CONFIRM, buffer, data.context_record2);
}

// The daily dialog's closing of a filed request.
void opeu_request_finish(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, RequestMenuService::FINISH, 3);
}

// Files the request in the current side's slot for its kind: "I will file the request immediately."
void opeu_request_file(uint8_t ds[65536], HostEvent event, uint16_t request) {
  auto&       data{ Data(ds) };
  auto const& filed{ Record<RequestSlot>(data, request) };
  auto&       slot{ FiledRequests(data)[filed.kind] };
  slot.target   = filed.target;
  slot.argument = filed.argument;
  slot.kind     = filed.kind;
  Call(ds, event, RequestMenuService::MESSAGE, offsetof(DataSegment, rq_filed_text), data.context_record2);
  Call(ds, event, RequestMenuService::SHOW, REQUEST_SHOW, data.current_side, FINISH_PRESENT_OFFSET, FINISH_PRESENT_SEGMENT);
}

namespace opeu::game::detail::rqcommon {
auto FileRequest(std::uint8_t* ds, RequestSlot request) -> void {
  ServiceLocals const locals{ sizeof(RequestSlot) };
  auto const          at{ locals.At(0) };
  Record<RequestSlot>(Data(ds), at) = request;
  opeu_request_file(ds, svc_rqcommon, at);
}
} // namespace opeu::game::detail::rqcommon

// Whether the current side holds at least one objective (city).
uint16_t opeu_side_has_city(const uint8_t ds[65536]) {
  auto const& data{ Data(ds) };
  for (auto const& objective : data.objectives)
    if (objective.active && opeu_objective_side(ds, Near(data, objective)) == data.current_side) return 1;
  return 0;
}

uint16_t svc_request(const char* module, uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (static_cast<RequestMenuService>(id)) {
  case RequestMenuService::PENDING:       r = opeu_request_pending(ds, (uint8_t)Arg(args, count, 0)); break;
  case RequestMenuService::CANCEL_PROMPT: r = opeu_request_cancel_prompt(ds, svc_rqcommon, Arg(args, count, 0)); break;
  case RequestMenuService::FILE:          FileRequest(ds, { Arg(args, count, 0), static_cast<uint8_t>(Arg(args, count, 1)), static_cast<uint8_t>(Arg(args, count, 2)) }); break;
  case RequestMenuService::MESSAGE:       opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestMenuService::TITLE:         opeu_army_title(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case RequestMenuService::DELAY_DIALOG:  r = opeu_delay_dialog(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case RequestMenuService::FORMAT:        opeu_request_format(ds, { args, count }); break;
  case RequestMenuService::CONFIRM:       r = opeu_confirm_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestMenuService::FINISH:        opeu_pending_finish(ds, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  case RequestMenuService::SHOW:          opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), Arg32(args, count, 2)); break;
  case RequestMenuService::PICK:          r = opeu_map_pick_location(ds, svc_mappick, Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestMenuService::POSITION:      opeu_army_position(ds, (TextHost*)port_textout_host(), (short)Arg(args, count, 0), (short)Arg(args, count, 1)); break;
  case RequestMenuService::PRINT:         opeu_cursor_print(ds, { args, count }); break;
  case RequestMenuService::MODE_IS:       r = opeu_request_mode_is(ds, (uint8_t)Arg(args, count, 0)); break;
  case RequestMenuService::HAS_CITY:      r = opeu_side_has_city(ds); break;
  case RequestMenuService::FAR_CALL:      r = (uint16_t)port_call(ds, (uint32_t)Arg(args, count, 1) * 16u + Arg(args, count, 0), 0, 0); break;
  default:                                port_unhandled(module, id, count);
  }
  return r;
}

uint16_t svc_rqcommon(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_request("rqcommon", ds, id, args, count);
}

auto port_routine_opeu_request_finish(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_request_finish(ds, svc_rqcommon);
  return 0;
}
