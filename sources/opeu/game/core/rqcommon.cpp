// The request menu's shared pieces: its service table and the helpers its choices use.
#include "core/rqcommon.hpp"
#include "common/memory.hpp"
#include "data-segment.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
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
void opeu_request_no_more(Game& game, HostEvent event, uint16_t what) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(game) };
  auto const          buffer{ locals.At(0) };
  Call(game, event, RequestMenuService::FORMAT, buffer, offsetof(DataSegment, rq_no_more_text), what);
  Call(game, event, RequestMenuService::MESSAGE, buffer, data.context_record3);
}

// Whether the current side has filed a request of this kind (supplies, battalion or regiment).
uint16_t opeu_request_pending(Game const& game, uint8_t kind) {
  return FiledRequests(Data(game))[kind].kind != std::to_underlying(RequestKind::NONE) ? 1 : 0;
}

// Whether the current side's booked request is of this kind.
uint16_t opeu_request_mode_is(Game const& game, uint8_t mode) {
  auto const& data{ Data(game) };
  return data.request_configs[data.current_side].kind == mode ? 1 : 0;
}

// "Your %s request has been sent. Cancel the request?"; returns the context's second person's answer.
uint16_t opeu_request_cancel_prompt(Game& game, HostEvent event, uint16_t what) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(game) };
  auto const          buffer{ locals.At(0) };
  Call(game, event, RequestMenuService::FORMAT, buffer, offsetof(DataSegment, rq_cancel_text), what);
  return Call(game, event, RequestMenuService::CONFIRM, buffer, data.context_record2);
}

// The daily dialog's closing of a filed request.
void opeu_request_finish(Game& game, HostEvent event) {
  Call(game, event, RequestMenuService::FINISH, 3);
}

// Files the request in the current side's slot for its kind: "I will file the request immediately."
void opeu_request_file(Game& game, HostEvent event, uint16_t request) {
  auto&       data{ Data(game) };
  auto const& filed{ Record<RequestSlot>(data, request) };
  auto&       slot{ FiledRequests(data)[filed.kind] };
  slot.target   = filed.target;
  slot.argument = filed.argument;
  slot.kind     = filed.kind;
  Call(game, event, RequestMenuService::MESSAGE, offsetof(DataSegment, rq_filed_text), data.context_record2);
  Call(game, event, RequestMenuService::SHOW, REQUEST_SHOW, data.current_side, FINISH_PRESENT_OFFSET, FINISH_PRESENT_SEGMENT);
}

namespace opeu::game::detail::rqcommon {
auto FileRequest(Game& game, RequestSlot request) -> void {
  ServiceLocals const locals{ sizeof(RequestSlot) };
  auto const          at{ locals.At(0) };
  Record<RequestSlot>(Data(game), at) = request;
  opeu_request_file(game, svc_rqcommon, at);
}
} // namespace opeu::game::detail::rqcommon

// Whether the current side holds at least one objective (city).
uint16_t opeu_side_has_city(Game const& game) {
  auto const& data{ Data(game) };
  for (auto const& objective : data.objectives)
    if (objective.active && opeu_objective_side(game, Near(data, objective)) == data.current_side) return 1;
  return 0;
}

uint16_t svc_request(const char* module, Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (static_cast<RequestMenuService>(id)) {
  case RequestMenuService::PENDING:       r = opeu_request_pending(game, (uint8_t)Arg(args, count, 0)); break;
  case RequestMenuService::CANCEL_PROMPT: r = opeu_request_cancel_prompt(game, svc_rqcommon, Arg(args, count, 0)); break;
  case RequestMenuService::FILE:          FileRequest(game, { Arg(args, count, 0), static_cast<uint8_t>(Arg(args, count, 1)), static_cast<uint8_t>(Arg(args, count, 2)) }); break;
  case RequestMenuService::MESSAGE:       opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestMenuService::TITLE:         opeu_army_title(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case RequestMenuService::DELAY_DIALOG: r = opeu_delay_dialog(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case RequestMenuService::FORMAT:       opeu_request_format(game, { args, count }); break;
  case RequestMenuService::CONFIRM:      r = opeu_confirm_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestMenuService::FINISH:       opeu_pending_finish(game, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  case RequestMenuService::SHOW:         opeu_daily_show(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), Arg32(args, count, 2)); break;
  case RequestMenuService::PICK:         r = opeu_map_pick_location(game, svc_mappick, Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestMenuService::POSITION:     opeu_army_position(game, (TextHost*)port_textout_host(), (short)Arg(args, count, 0), (short)Arg(args, count, 1)); break;
  case RequestMenuService::PRINT:        opeu_cursor_print(game, { args, count }); break;
  case RequestMenuService::MODE_IS:      r = opeu_request_mode_is(game, (uint8_t)Arg(args, count, 0)); break;
  case RequestMenuService::HAS_CITY:     r = opeu_side_has_city(game); break;
  case RequestMenuService::FAR_CALL:     r = (uint16_t)port_call(game, (uint32_t)Arg(args, count, 1) * 16u + Arg(args, count, 0), 0, 0); break;
  default:                               port_unhandled(module, id, count);
  }
  return r;
}

uint16_t svc_rqcommon(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_request("rqcommon", game, id, args, count);
}

auto port_routine_opeu_request_finish(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_request_finish(game, svc_rqcommon);
  return 0;
}
