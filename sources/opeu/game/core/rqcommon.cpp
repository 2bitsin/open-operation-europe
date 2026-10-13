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
#include "map/mappick.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <utility>

using namespace opeu::game;

namespace {

constexpr uint16_t TEXT_BYTES{ 0x7c }; // the prompt text the format service writes
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
void opeu_request_no_more(Game& game, uint16_t what) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(game) };
  auto const          buffer{ locals.At(0) };
  opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(offsetof(DataSegment, rq_no_more_text)), static_cast<std::uint16_t>(what) });
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), buffer, data.context_record3);
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
uint16_t opeu_request_cancel_prompt(Game& game, uint16_t what) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(game) };
  auto const          buffer{ locals.At(0) };
  opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(offsetof(DataSegment, rq_cancel_text)), static_cast<std::uint16_t>(what) });
  return opeu_confirm_message(game, static_cast<const DialogHost*>(port_dialogs_host()), buffer, data.context_record2);
}

// The daily dialog's closing of a filed request.
void opeu_request_finish(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(3));
}

// Files the request in the current side's slot for its kind: "I will file the request immediately."
void opeu_request_file(Game& game, uint16_t request) {
  auto&       data{ Data(game) };
  auto const& filed{ Record<RequestSlot>(data, request) };
  auto&       slot{ FiledRequests(data)[filed.kind] };
  slot.target   = filed.target;
  slot.argument = filed.argument;
  slot.kind     = filed.kind;
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, rq_filed_text), data.context_record2);
  opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), REQUEST_SHOW, static_cast<uint8_t>(data.current_side), Presenter::REQUEST_FINISH);
}

namespace opeu::game::detail::rqcommon {
auto FileRequest(Game& game, RequestSlot request) -> void {
  ServiceLocals const locals{ sizeof(RequestSlot) };
  auto const          at{ locals.At(0) };
  Record<RequestSlot>(Data(game), at) = request;
  opeu_request_file(game, at);
}
} // namespace opeu::game::detail::rqcommon

// Whether the current side holds at least one objective (city).
uint16_t opeu_side_has_city(Game const& game) {
  auto const& data{ Data(game) };
  for (auto const& objective : data.objectives)
    if (objective.active && opeu_objective_side(game, Near(data, objective)) == data.current_side) return 1;
  return 0;
}
