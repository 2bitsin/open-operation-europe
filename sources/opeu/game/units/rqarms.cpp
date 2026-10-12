// The battalion (arms) request of the request menu, the always-true row predicate of the regiment list, and the
// services the menu's requests share.
#include "units/rqarms.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "core/daylist.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/random.hpp"
#include "core/recindex.hpp"
#include "core/rqcommon.hpp"
#include "glue.hpp"
#include "io/listdlg.hpp"
#include "map/mappick.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/release.hpp"
#include "units/value.hpp"

#include <utility>

using namespace opeu::game;

namespace {
// The request's frame on the DS stack: the picked cell's x and y bytes, then the confirmation text.
constexpr std::uint16_t REQUEST_SCRATCH{ 0x80 };
constexpr std::uint16_t CONFIRM_BUFFER{ 4 };
// The request-menu prompts keep their text buffer at the same place in their own frame.
constexpr std::uint16_t PICK_FRAME{ 2 };
} // namespace

// "What kind of battalion would you like sent?": cancels a filed battalion request, or has the player choose one of
// the side's available equipment types and the own city that receives it, then files the request.
void opeu_request_arms(uint8_t ds[65536], HostEvent event, uint16_t scratch) {
  auto&      data{ Data(ds) };
  auto&      x{ Record<std::uint8_t>(data, scratch) };
  auto&      y{ Record<std::uint8_t>(data, static_cast<std::uint16_t>(scratch + 1)) };
  auto const buffer{ static_cast<std::uint16_t>(scratch + CONFIRM_BUFFER) };
  auto const battalion{ std::to_underlying(RequestKind::BATTALION) };
  if (Call(ds, event, RequestService::PENDING, battalion)) {
    if (!Call(ds, event, RequestService::CANCEL_PROMPT, Near(data, data.rq_arms_text))) return;
    Record<Side>(data, data.current_side_ptr).requests[battalion].kind = std::to_underlying(RequestKind::NONE);
  }
  Call(ds, event, RequestService::CONTROL_PANEL);
  std::uint8_t count{ 0 };
  for (std::uint8_t category{ 0 }; category < 8; ++category)
    count += static_cast<std::uint8_t>(Call(ds, event, RequestService::ELEMENT_TYPES,
                                            data.side_masks[data.current_side], category,
                                            Near(data, data.rq_type_list[count])));
  data.rq_type_dialog.rows = static_cast<std::uint8_t>(count + 1);
  Call(ds, event, RequestService::CONTROL_MESSAGE, Near(data, data.rq_arms_which), data.context_record2);
  if (!Call(ds, event, RequestService::LIST_DIALOG, Near(data, data.rq_type_dialog), 1, 7)) return;
  x = static_cast<std::uint8_t>(data.viewport_x);
  y = static_cast<std::uint8_t>(data.viewport_y);
  Call(ds, event, RequestService::CONTROL_MESSAGE, Near(data, data.rq_arms_where), data.context_record2);
  std::uint16_t city;
  for (;;) {
    if (!Call(ds, event, RequestService::MAP_PICK_LOCATION, scratch, static_cast<std::uint16_t>(scratch + 1))) return;
    city = opeu_find_objective_at(ds, x, y);
    if (city && opeu_objective_side(ds, city) == data.current_side) break;
  }
  Call(ds, event, RequestService::REQUEST_FORMAT, buffer, Near(data, data.rq_arms_confirm), city);
  if (!Call(ds, event, RequestService::CONFIRM_MESSAGE, buffer, data.context_record2)) return;
  Call(ds, event, RequestService::FILE_REQUEST, city, data.rq_type_list[data.list_row], battalion);
  data.side_delays[data.current_side] = 1;
}

// The row predicate of the regiment table: every row is selectable.
uint8_t opeu_rq_unit_selectable(void) {
  return 1;
}

// The services of the battalion request.
uint16_t svc_rqarms(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
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
  case RequestService::ELEMENT_TYPES:     return svc_units_request("rqarms", ds, id, args, count);
  default:                                port_unhandled("rqarms", id, count);
  }
  return 0;
}

// The services of the battalion and regiment requests; `module` names the caller in diagnostics.
uint16_t svc_units_request(const char* module, uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  auto&    data{ Data(ds) };
  uint16_t r = 0;
  switch (static_cast<RequestService>(id)) {
  case RequestService::RANDOM:  r = opeu_rand_bounded(Arg(args, count, 0)); break;
  case RequestService::MIN16:   r = opeu_min16(Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestService::MAX16:   r = opeu_max16(Arg(args, count, 0), Arg(args, count, 1)); break;
  case RequestService::PENDING: r = opeu_request_pending(ds, (uint8_t)Arg(args, count, 0)); break;
  case RequestService::CANCEL_PROMPT:
    r = WithStack(SERVICE_STACK, [&](std::uint16_t sp) -> std::uint16_t {
      return opeu_request_cancel_prompt(ds, svc_rqcommon, (uint16_t)(sp - REQUEST_SCRATCH), Arg(args, count, 0));
    });
    break;
  case RequestService::NO_MORE:
    WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
      opeu_request_no_more(ds, svc_rqcommon, (uint16_t)(sp - REQUEST_SCRATCH), Arg(args, count, 0));
    });
    break;
  case RequestService::FILE_REQUEST:
    // the request is filed from a copy on the caller's frame
    WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
      auto const at{ static_cast<std::uint16_t>(sp - sizeof(RequestSlot)) };
      Record<RequestSlot>(data, at) = { Arg(args, count, 0), (uint8_t)Arg(args, count, 1),
                                        (uint8_t)Arg(args, count, 2) };
      opeu_request_file(ds, svc_rqcommon, at);
    });
    break;
  case RequestService::FREE_UNITS:
    r = opeu_release_side_units(ds, Arg(args, count, 0), (uint8_t)Arg(args, count, 1));
    break;
  case RequestService::FREE_ELEMENTS:
    r = opeu_release_side_elements(ds, Arg(args, count, 0), (uint8_t)Arg(args, count, 1));
    break;
  case RequestService::CONTROL_PANEL: opeu_title_panel_begin(ds, (const DialogHost*)port_dialogs_host()); break;
  case RequestService::CONTROL_MESSAGE:
    opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1));
    break;
  case RequestService::LIST_DIALOG:
    r = opeu_list_dialog(ds, svc_listdlg, Arg(args, count, 0), (uint8_t)Arg(args, count, 1),
                         (uint8_t)Arg(args, count, 2));
    break;
  case RequestService::UNIT_INDEX:      r = opeu_unit_index(Arg(args, count, 0)); break;
  case RequestService::CATEGORY_0_TO_3: r = opeu_category_0_to_3((uint8_t)Arg(args, count, 0)); break;
  case RequestService::CATEGORY_4_OR_5: r = opeu_category_4_or_5((uint8_t)Arg(args, count, 0)); break;
  case RequestService::CATEGORY_6:      r = opeu_category_6((uint8_t)Arg(args, count, 0)); break;
  case RequestService::ARMY_TITLE:
    opeu_army_title(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0));
    break;
  case RequestService::DELAY_DIALOG:
    r = opeu_delay_dialog(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1),
                          Arg(args, count, 2), Arg(args, count, 3));
    break;
  case RequestService::CONFIRM_OK:
    r = opeu_confirm_ok(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0));
    break;
  case RequestService::REQUEST_FORMAT: r = lf_request_format(ds, args, count); break;
  case RequestService::CONFIRM_MESSAGE:
    r = opeu_confirm_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1));
    break;
  case RequestService::MAP_PICK_LOCATION:
    r = WithStack(SERVICE_STACK, [&](std::uint16_t sp) -> std::uint16_t {
      return opeu_map_pick_location(ds, svc_mappick, (uint16_t)(sp - PICK_FRAME), Arg(args, count, 0),
                                    Arg(args, count, 1));
    });
    break;
  case RequestService::ELEMENT_TYPES:
    r = opeu_list_due_records(ds, Arg(args, count, 2), (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1));
    break;
  default: port_unhandled(module, id, count);
  }
  return r;
}

auto port_routine_opeu_request_arms(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  WithStack(SERVICE_STACK,
            [&](std::uint16_t sp) { opeu_request_arms(ds, svc_rqarms, (uint16_t)(sp - REQUEST_SCRATCH)); });
  return 0;
}

auto port_routine_opeu_rq_unit_selectable(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)ds;
  (void)args;
  (void)count;
  return opeu_rq_unit_selectable();
}
