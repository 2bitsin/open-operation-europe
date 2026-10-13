// The request menu's supplies choice, and the row predicate of the battalion type list.
#include "core/rqsupply.hpp"
#include "common/memory.hpp"
#include "core/rqcommon.hpp"
#include "data-segment.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {

constexpr uint16_t SCRATCH_BYTES{ 0x40 }; // the DS scratch the caller reserves for the supplies choice
// The supplies choice's reserve: the picked cell, then the confirmation text.
constexpr uint16_t CELL_AT{ 0 };
constexpr uint16_t BUFFER_OFFSET{ 4 };

// The map cell the player picks, kept in the reserve because the pick service writes it through its address.
struct PickedCell {
  uint8_t x;
  uint8_t y;
};

} // namespace

// "Which city would you like to receive the supplies?": a filed supplies request may be cancelled first; then the
// player picks one of the side's own cities on the map, confirms, and the request is filed and the side's turn ends.
void opeu_request_supplies(Game& game, HostEvent event) {
  ServiceLocals const locals{ SCRATCH_BYTES };
  auto const          scratch{ locals.At(CELL_AT) };
  auto&               data{ Data(game) };
  auto&               cell{ Record<PickedCell>(data, scratch) };
  auto const          buffer{ static_cast<uint16_t>(scratch + BUFFER_OFFSET) };
  auto const          kind{ std::to_underlying(RequestKind::SUPPLIES) };
  if (Call(game, event, RequestMenuService::PENDING, kind)) {
    if (!Call(game, event, RequestMenuService::CANCEL_PROMPT, offsetof(DataSegment, rq_supplies_text))) return;
    Record<Side>(data, data.current_side_ptr).requests[kind].kind = std::to_underlying(RequestKind::NONE);
  }
  Call(game, event, RequestMenuService::TITLE, offsetof(DataSegment, rq_supplies_title));
  cell.x = data.viewport_x_bytes[0];
  cell.y = data.viewport_y_bytes[0];
  Call(game, event, RequestMenuService::MESSAGE, offsetof(DataSegment, rq_supplies_where), data.context_record2);
  uint16_t city;
  for (;;) {
    if (!Call(game, event, RequestMenuService::PICK, scratch, scratch + 1)) return;
    city = opeu_find_objective_at(game, cell.x, cell.y);
    if (city && opeu_objective_side(game, city) == data.current_side) break;
  }
  Call(game, event, RequestMenuService::FORMAT, buffer, offsetof(DataSegment, rq_supplies_confirm), city);
  if (!Call(game, event, RequestMenuService::CONFIRM, buffer, data.context_record2)) return;
  Call(game, event, RequestMenuService::FILE, city, 0xff, kind);
  data.side_delays[data.current_side] = 1;
}

// The row predicate of the battalion type list: every row can be chosen.
uint8_t opeu_rq_type_selectable() {
  return 1;
}

uint16_t svc_rqsupply(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_request("rqsupply", game, id, args, count);
}

auto port_routine_opeu_request_supplies(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_request_supplies(game, svc_rqsupply);
  return 0;
}

auto port_routine_opeu_rq_type_selectable(Game&, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_rq_type_selectable();
}
