// Whether a filed request can still be carried out when its day comes.
#include "core/reqvalid.hpp"
#include "core/reqcheck.hpp"
#include "data-segment.hpp"
#include "map/arrplace.hpp"
#include "map/cellpred.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "map/reqcells.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {

// The daily dialog of the request reports.
constexpr uint16_t REQUEST_SHOW{ 0x73 };

// The reasons a request fails (request_error), each with its message (core/reqerror).
enum class RequestError : uint8_t {
  LOST     = 0, // the target objective is no longer the side's
  NO_PLACE = 1, // no cell to deliver to or to place the regiment at
  NO_ROOM  = 2, // no item slot left at the objective, or the regiment's division is not fielded
};

} // namespace

// Checks a request of the side for supplies, a battalion or a regiment. A failed request
// is noted (invalid_request, request_error) and reported in the side's daily dialog; returns 1 when it is valid.
uint16_t opeu_request_valid(Game& game, uint8_t side, uint16_t request) {
  auto&      data{ Data(game) };
  auto&      slot{ Record<RequestSlot>(data, request) };
  auto const fail{ [&](RequestError reason) {
    data.request_error   = std::to_underlying(reason);
    data.invalid_request = request;
    opeu_daily_show(game, REQUEST_SHOW, side, Presenter::REQUEST_ERROR);
    return uint16_t{ 0 };
  } };
  auto const count_cells{ [&] { return opeu_request_cells(game, side, opeu_request_cell_allowed); } };
  switch (static_cast<RequestKind>(slot.kind)) {
  case RequestKind::SUPPLIES:
    if (opeu_objective_side(game, slot.target) != side) return fail(RequestError::LOST);
    if (!count_cells()) return fail(RequestError::NO_PLACE);
    return 1;
  case RequestKind::BATTALION:
    if (opeu_objective_side(game, slot.target) != side) return fail(RequestError::LOST);
    if (!count_cells()) return fail(RequestError::NO_PLACE);
    if (!opeu_request_slot(game, slot.target, slot.argument)) return fail(RequestError::NO_ROOM);
    return 1;
  case RequestKind::REGIMENT:
    if (!opeu_request_group_ready(game, side)) return fail(RequestError::NO_ROOM);
    if (!opeu_arrival_locate(game, offsetof(DataSegment, request_x), offsetof(DataSegment, request_y), side)) return fail(RequestError::NO_PLACE);
    return 1;
  default: return 0;
  }
}
