// Whether a filed request can still be carried out when its day comes.
#include "core/reqvalid.hpp"
#include "core/reqcheck.hpp"
#include "data-segment.hpp"
#include "map/arrplace.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "core/dialogs.hpp"
#include "glue.hpp"
#include "map/reqcells.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {

// The services opeu_request_valid calls through its event (opeu_arrival_locate calls the first one too).
enum class ValidService : uint16_t {
  COUNT_CELLS = 0, // opeu_request_cells(side, predicate offset, predicate segment)
  SHOW        = 1, // opeu_daily_show(id, side, present offset, present segment)
};

// The far address of opeu_request_cell_allowed (map/reqcells), the predicate of the cells a supply drop may use.
constexpr uint16_t CELL_ALLOWED_OFFSET{ 0x1114 };
constexpr uint16_t CELL_ALLOWED_SEGMENT{ 0x145c };
// The far address of opeu_request_error_present (core/reqerror), which SHOW calls back.
constexpr uint16_t ERROR_PRESENT_OFFSET{ 0x2e4 };
constexpr uint16_t ERROR_PRESENT_SEGMENT{ 0x1270 };
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
uint16_t opeu_request_valid(uint8_t ds[65536], PendingEvent event, uint8_t side, uint16_t request) {
  auto&      data{ Data(ds) };
  auto&      slot{ Record<RequestSlot>(data, request) };
  auto const fail{ [&](RequestError reason) {
    data.request_error   = std::to_underlying(reason);
    data.invalid_request = request;
    event(ds, std::to_underlying(ValidService::SHOW), REQUEST_SHOW, side, ERROR_PRESENT_OFFSET, ERROR_PRESENT_SEGMENT);
    return uint16_t{ 0 };
  } };
  auto const count_cells{ [&] {
    return event(ds, std::to_underlying(ValidService::COUNT_CELLS), side, CELL_ALLOWED_OFFSET, CELL_ALLOWED_SEGMENT, 0);
  } };
  switch (static_cast<RequestKind>(slot.kind)) {
  case RequestKind::SUPPLIES:
    if (opeu_objective_side(ds, slot.target) != side) return fail(RequestError::LOST);
    if (!count_cells()) return fail(RequestError::NO_PLACE);
    return 1;
  case RequestKind::BATTALION:
    if (opeu_objective_side(ds, slot.target) != side) return fail(RequestError::LOST);
    if (!count_cells()) return fail(RequestError::NO_PLACE);
    if (!opeu_request_slot(ds, slot.target, slot.argument)) return fail(RequestError::NO_ROOM);
    return 1;
  case RequestKind::REGIMENT:
    if (!opeu_request_group_ready(ds, side)) return fail(RequestError::NO_ROOM);
    if (!opeu_arrival_locate(ds, event, offsetof(DataSegment, request_x), offsetof(DataSegment, request_y), side))
      return fail(RequestError::NO_PLACE);
    return 1;
  default: return 0;
  }
}

uint16_t svc_reqvalid(uint8_t* ds, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  uint16_t r = 0;
  switch (static_cast<ValidService>(id)) {
  case ValidService::COUNT_CELLS: r = opeu_request_cells(ds, (uint8_t)a, c2_cell_predicate(b, c)); break;
  case ValidService::SHOW:
    opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), a, (uint8_t)b, (uint32_t)d << 16 | c);
    break;
  default: port_unhandled("reqvalid", id, 4);
  }
  return r;
}
