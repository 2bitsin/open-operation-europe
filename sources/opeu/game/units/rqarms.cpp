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

#include <array>
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
void opeu_request_arms(Game& game) {
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ REQUEST_SCRATCH };
  auto const          scratch{ buffers.At(0) };
  auto&               x{ Record<std::uint8_t>(data, scratch) };
  auto&               y{ Record<std::uint8_t>(data, static_cast<std::uint16_t>(scratch + 1)) };
  auto const          buffer{ static_cast<std::uint16_t>(scratch + CONFIRM_BUFFER) };
  auto const          battalion{ std::to_underlying(RequestKind::BATTALION) };
  if (opeu_request_pending(game, static_cast<uint8_t>(battalion))) {
    if (!opeu_request_cancel_prompt(game, Near(data, data.rq_arms_text))) return;
    Record<Side>(data, data.current_side_ptr).requests[battalion].kind = std::to_underlying(RequestKind::NONE);
  }
  opeu_title_panel_begin(game, static_cast<const DialogHost*>(port_dialogs_host()));
  std::uint8_t count{ 0 };
  for (std::uint8_t category{ 0 }; category < 8; ++category)
    count += static_cast<std::uint8_t>(opeu_list_due_records(game, Near(data, data.rq_type_list[count]), static_cast<uint8_t>(data.side_masks[data.current_side]), static_cast<uint8_t>(category)));
  data.rq_type_dialog.rows = static_cast<std::uint8_t>(count + 1);
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.rq_arms_which), data.context_record2);
  if (!opeu_list_dialog(game, Near(data, data.rq_type_dialog), static_cast<uint8_t>(LIST_LIMIT), static_cast<uint8_t>(LIST_FLAGS))) return;
  x = static_cast<std::uint8_t>(data.viewport_x);
  y = static_cast<std::uint8_t>(data.viewport_y);
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.rq_arms_where), data.context_record2);
  std::uint16_t city;
  for (;;) {
    if (!opeu_map_pick_location(game, scratch, static_cast<std::uint16_t>(scratch + 1))) return;
    city = opeu_find_objective_at(game, x, y);
    if (city && opeu_objective_side(game, city) == data.current_side) break;
  }
  opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(Near(data, data.rq_arms_confirm)), static_cast<std::uint16_t>(city) });
  if (!opeu_confirm_message(game, static_cast<const DialogHost*>(port_dialogs_host()), buffer, data.context_record2)) return;
  FileRequest(game, { city, static_cast<uint8_t>(data.rq_type_list[data.list_row]), static_cast<uint8_t>(battalion) });
  data.side_delays[data.current_side] = 1;
}

// The row predicate of the regiment table: every row is selectable.
uint8_t opeu_rq_unit_selectable(void) {
  return 1;
}
