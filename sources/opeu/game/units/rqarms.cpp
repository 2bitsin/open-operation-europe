// The battalion (arms) request of the request menu, the always-true row predicate of the regiment list, and the
// services the menu's requests share.
#include "units/rqarms.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/daylist.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/random.hpp"
#include "core/recindex.hpp"
#include "core/rqcommon.hpp"
#include "io/listdlg.hpp"
#include "link/glue.hpp"
#include "map/mappick.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/release.hpp"
#include "units/value.hpp"

#include <utility>

using namespace opeu::game;

namespace {
// A request's buffers in the data segment (its own reserve, and the frame the request services are given): the picked
// cell's x and y bytes, then the confirmation text.
constexpr std::uint16_t REQUEST_SCRATCH{ 0x80 };
constexpr std::uint16_t CONFIRM_BUFFER{ 4 };
} // namespace

// "What kind of battalion would you like sent?": cancels a filed battalion request, or has the player choose one of
// the side's available equipment types and the own city that receives it, then files the request.
void opeu_request_arms(Game& game, HostEvent event) {
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ REQUEST_SCRATCH };
  auto const          scratch{ buffers.At(0) };
  auto&               x{ Record<std::uint8_t>(data, scratch) };
  auto&               y{ Record<std::uint8_t>(data, static_cast<std::uint16_t>(scratch + 1)) };
  auto const          buffer{ static_cast<std::uint16_t>(scratch + CONFIRM_BUFFER) };
  auto const          battalion{ std::to_underlying(RequestKind::BATTALION) };
  if (Call(game, event, RequestService::PENDING, battalion)) {
    if (!Call(game, event, RequestService::CANCEL_PROMPT, Near(data, data.rq_arms_text))) return;
    Record<Side>(data, data.current_side_ptr).requests[battalion].kind = std::to_underlying(RequestKind::NONE);
  }
  Call(game, event, RequestService::CONTROL_PANEL);
  std::uint8_t count{ 0 };
  for (std::uint8_t category{ 0 }; category < 8; ++category)
    count += static_cast<std::uint8_t>(Call(game, event, RequestService::ELEMENT_TYPES, data.side_masks[data.current_side], category, Near(data, data.rq_type_list[count])));
  data.rq_type_dialog.rows = static_cast<std::uint8_t>(count + 1);
  Call(game, event, RequestService::CONTROL_MESSAGE, Near(data, data.rq_arms_which), data.context_record2);
  if (!Call(game, event, RequestService::LIST_DIALOG, Near(data, data.rq_type_dialog), LIST_LIMIT, LIST_FLAGS)) return;
  x = static_cast<std::uint8_t>(data.viewport_x);
  y = static_cast<std::uint8_t>(data.viewport_y);
  Call(game, event, RequestService::CONTROL_MESSAGE, Near(data, data.rq_arms_where), data.context_record2);
  std::uint16_t city;
  for (;;) {
    if (!Call(game, event, RequestService::MAP_PICK_LOCATION, scratch, static_cast<std::uint16_t>(scratch + 1))) return;
    city = opeu_find_objective_at(game, x, y);
    if (city && opeu_objective_side(game, city) == data.current_side) break;
  }
  Call(game, event, RequestService::REQUEST_FORMAT, buffer, Near(data, data.rq_arms_confirm), city);
  if (!Call(game, event, RequestService::CONFIRM_MESSAGE, buffer, data.context_record2)) return;
  Call(game, event, RequestService::FILE_REQUEST, city, data.rq_type_list[data.list_row], battalion);
  data.side_delays[data.current_side] = 1;
}

// The row predicate of the regiment table: every row is selectable.
uint8_t opeu_rq_unit_selectable(void) {
  return 1;
}

// The services of the battalion request.
uint16_t svc_rqarms(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (static_cast<RequestService>(id)) {
  case RequestService::PENDING:
  case RequestService::CANCEL_PROMPT:
  case RequestService::FILE_REQUEST:
  case RequestService::CONTROL_PANEL:
  case RequestService::CONTROL_MESSAGE:
  case RequestService::LIST_DIALOG:
  case RequestService::REQUEST_FORMAT:
  case RequestService::CONFIRM_MESSAGE:
  case RequestService::MAP_PICK_LOCATION:
  case RequestService::ELEMENT_TYPES:     return svc_units_request("rqarms", game, id, args, count);
  default:                                port_unhandled("rqarms", id, count);
  }
  return 0;
}

// The services of the battalion and regiment requests; `module` names the caller in diagnostics.
uint16_t svc_units_request(const char* module, Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (static_cast<RequestService>(id)) {
  case RequestService::RANDOM:          r = opeu_rand_bounded(Arg(args, count, 0)); break;
  case RequestService::MIN16:           r = opeu_min16(Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestService::MAX16:           r = opeu_max16(Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestService::PENDING:         r = opeu_request_pending(game, (uint8_t)Arg(args, count, 0)); break;
  case RequestService::CANCEL_PROMPT:   r = opeu_request_cancel_prompt(game, svc_rqcommon, Arg(args, count, 0)); break;
  case RequestService::NO_MORE:         opeu_request_no_more(game, svc_rqcommon, Arg(args, count, 0)); break;
  case RequestService::FILE_REQUEST:    FileRequest(game, { Arg(args, count, 0), static_cast<uint8_t>(Arg(args, count, 1)), static_cast<uint8_t>(Arg(args, count, 2)) }); break;
  case RequestService::FREE_UNITS:      r = opeu_release_side_units(game, Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case RequestService::FREE_ELEMENTS:   r = opeu_release_side_elements(game, Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case RequestService::CONTROL_PANEL:   opeu_title_panel_begin(game, (const DialogHost*)port_dialogs_host()); break;
  case RequestService::CONTROL_MESSAGE: opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestService::LIST_DIALOG:     r = opeu_list_dialog(game, svc_listdlg, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case RequestService::UNIT_INDEX:      r = opeu_unit_index(Arg(args, count, 0)); break;
  case RequestService::CATEGORY_0_TO_3: r = opeu_category_0_to_3((uint8_t)Arg(args, count, 0)); break;
  case RequestService::CATEGORY_4_OR_5: r = opeu_category_4_or_5((uint8_t)Arg(args, count, 0)); break;
  case RequestService::CATEGORY_6:      r = opeu_category_6((uint8_t)Arg(args, count, 0)); break;
  case RequestService::ARMY_TITLE:      opeu_army_title(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case RequestService::DELAY_DIALOG:    r = opeu_delay_dialog(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case RequestService::CONFIRM_OK:      r = opeu_confirm_ok(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case RequestService::REQUEST_FORMAT:  opeu_request_format(game, { args, count }); break;
  case RequestService::CONFIRM_MESSAGE: r = opeu_confirm_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestService::MAP_PICK_LOCATION: r = opeu_map_pick_location(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestService::ELEMENT_TYPES:     r = opeu_list_due_records(game, Arg(args, count, 2), (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  default:                                port_unhandled(module, id, count);
  }
  return r;
}

auto port_routine_opeu_request_arms(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_request_arms(game, svc_rqarms);
  return 0;
}

auto port_routine_opeu_rq_unit_selectable(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)game;
  (void)args;
  (void)count;
  return opeu_rq_unit_selectable();
}
