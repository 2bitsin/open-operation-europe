// The request menu's supplies choice, and the row predicate of the battalion type list.
#include "core/rqsupply.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/rqcommon.hpp"
#include "core/textout.hpp"
#include "data-segment.hpp"
#include "map/mappick.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
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
void opeu_request_supplies(Game& game) {
  ServiceLocals const locals{ SCRATCH_BYTES };
  auto const          scratch{ locals.At(CELL_AT) };
  auto&               data{ Data(game) };
  auto&               cell{ Record<PickedCell>(data, scratch) };
  auto const          buffer{ static_cast<uint16_t>(scratch + BUFFER_OFFSET) };
  auto const          kind{ std::to_underlying(RequestKind::SUPPLIES) };
  if (opeu_request_pending(game, kind)) {
    if (!opeu_request_cancel_prompt(game, offsetof(DataSegment, rq_supplies_text))) return;
    Record<Side>(data, data.current_side_ptr).requests[kind].kind = std::to_underlying(RequestKind::NONE);
  }
  opeu_army_title(game, offsetof(DataSegment, rq_supplies_title));
  cell.x = data.viewport_x_bytes[0];
  cell.y = data.viewport_y_bytes[0];
  opeu_control_message(game, offsetof(DataSegment, rq_supplies_where), data.context_record2);
  uint16_t city;
  for (;;) {
    if (!opeu_map_pick_location(game, scratch, scratch + 1)) return;
    city = opeu_find_objective_at(game, cell.x, cell.y);
    if (city && opeu_objective_side(game, city) == data.current_side) break;
  }
  opeu_request_format(game, std::array<std::uint16_t, 3>{ buffer, static_cast<std::uint16_t>(offsetof(DataSegment, rq_supplies_confirm)), city });
  if (!opeu_confirm_message(game, buffer, data.context_record2)) return;
  FileRequest(game, { city, static_cast<uint8_t>(0xff), kind });
  data.side_delays[data.current_side] = 1;
}

// The row predicate of the battalion type list: every row can be chosen.
uint8_t opeu_rq_type_selectable() {
  return 1;
}
