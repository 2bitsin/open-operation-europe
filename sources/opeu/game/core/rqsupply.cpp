// The request menu's supplies choice, and the row predicate of the battalion type list.
#include "core/rqsupply.hpp"
#include "common/memory.hpp"
#include "core/rqcommon.hpp"
#include "data-segment.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {

constexpr uint16_t SCRATCH_BYTES{ 0x40 }; // the DS scratch of the supplies choice: the picked cell and the text
constexpr uint16_t BUFFER_OFFSET{ 4 }; // where in the scratch the text goes

// The map cell the player picks, kept in the scratch because the pick service writes it through its address.
struct PickedCell {
  uint8_t x;
  uint8_t y;
};

} // namespace

// "Which city would you like to receive the supplies?": a filed supplies request may be cancelled first; then the
// player picks one of the side's own cities on the map, confirms, and the request is filed and the side's turn ends.
void opeu_request_supplies(uint8_t ds[65536], HostEvent event, uint16_t scratch) {
  auto&      data{ Data(ds) };
  auto&      cell{ Record<PickedCell>(data, scratch) };
  auto const buffer{ static_cast<uint16_t>(scratch + BUFFER_OFFSET) };
  auto const kind{ std::to_underlying(RequestKind::SUPPLIES) };
  if (Call(ds, event, RequestMenuService::PENDING, kind)) {
    if (!Call(ds, event, RequestMenuService::CANCEL_PROMPT, offsetof(DataSegment, rq_supplies_text))) return;
    Record<Side>(data, data.current_side_ptr).requests[kind].kind = std::to_underlying(RequestKind::NONE);
  }
  Call(ds, event, RequestMenuService::TITLE, offsetof(DataSegment, rq_supplies_title));
  cell.x = data.viewport_x_bytes[0];
  cell.y = data.viewport_y_bytes[0];
  Call(ds, event, RequestMenuService::MESSAGE, offsetof(DataSegment, rq_supplies_where), data.context_record2);
  uint16_t city;
  for (;;) {
    if (!Call(ds, event, RequestMenuService::PICK, scratch, scratch + 1)) return;
    city = opeu_find_objective_at(ds, cell.x, cell.y);
    if (city && opeu_objective_side(ds, city) == data.current_side) break;
  }
  Call(ds, event, RequestMenuService::FORMAT, buffer, offsetof(DataSegment, rq_supplies_confirm), city);
  if (!Call(ds, event, RequestMenuService::CONFIRM, buffer, data.context_record2)) return;
  Call(ds, event, RequestMenuService::FILE, city, 0xff, kind);
  data.side_delays[data.current_side] = 1;
}

// The row predicate of the battalion type list: every row can be chosen.
uint8_t opeu_rq_type_selectable() {
  return 1;
}

uint16_t svc_rqsupply(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_request("rqsupply", ds, id, args, count);
}

auto port_routine_opeu_request_supplies(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  auto const scratch{ port_scratch_enter(SCRATCH_BYTES) };
  opeu_request_supplies(ds, svc_rqsupply, scratch);
  port_scratch_leave(SCRATCH_BYTES);
  return 0;
}

auto port_routine_opeu_rq_type_selectable(std::uint8_t*, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_rq_type_selectable();
}
