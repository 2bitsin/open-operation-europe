// The hourly handling of the sides' requests: a booked request is carried out when its delay runs out, and each hour a
// filed request may reach headquarters, which accepts or turns it down.
#include "types.hpp"
#include "core/requests.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/template.hpp"
#include "data-segment.hpp"
#include "common/random.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "core/reqcheck.hpp"
#include "core/reqequip.hpp"
#include "core/reqpend.hpp"
#include "core/reqres.hpp"
#include "core/reqspawn.hpp"
#include "core/reqvalid.hpp"

#include <utility>

using namespace opeu::game;

namespace {

// The services opeu_hourly_c_stage_b calls through its event.
enum class RequestService : uint16_t {
  VALID     = 0, // opeu_request_valid(side, request)
  SUPPLIES  = 1, // opeu_request_action0(side, request)
  BATTALION = 2, // opeu_request_action1(side, request)
  REGIMENT  = 3, // opeu_request_action2(side, request)
  COMPLETE  = 4, // opeu_request_complete(side)
  START     = 5, // opeu_request_start(side, request)
  FALLBACK  = 6, // opeu_request_fallback(side, request)
};

auto Send(uint8_t* ds, PendingEvent event, RequestService service, uint8_t side,
          uint16_t request = 0) -> uint16_t {
  return event(ds, std::to_underlying(service), side, request, 0, 0);
}

constexpr uint8_t TEMPLATE_KIND{
  5
}; // the person template that speaks the request reports

} // namespace

// For each side: the request booked in its configuration counts its delay down and, when it runs out, is carried out
// if it is still valid; then the first filed request that the hour's one-in-six roll picks reaches headquarters and is
// accepted when its kind is enabled, no other request is booked and a roll in 0..255 falls below the acceptance
// threshold of its kind. The threshold of an unknown kind is the caller's stale_threshold (the game reads an
// uninitialised word there).
void opeu_hourly_c_stage_b(uint8_t ds[65536], PendingEvent event,
                           uint16_t stale_threshold) {
  auto& data{ Data(ds) };
  auto  threshold{ stale_threshold };
  for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
    auto&      config{ data.request_configs[side] };
    auto const config_at{ Near(data, config) };
    data.request_template = opeu_find_person_template(
        ds,
        Record<Person>(
            data, opeu_find_side_record(
                      ds, side, std::to_underlying(StaffRole::ARMY_COMMANDER)))
            .status,
        TEMPLATE_KIND);
    data.request_person = opeu_find_side_record(
        ds, side, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF));
    if (config.kind != std::to_underlying(RequestKind::NONE) &&
        opeu_sub_byte(1, &config.delay) == 0) {
      if (Send(ds, event, RequestService::VALID, side, config_at)) {
        switch (static_cast<RequestKind>(config.kind)) {
          case RequestKind::SUPPLIES:
            Send(ds, event, RequestService::SUPPLIES, side, config_at);
            break;
          case RequestKind::BATTALION:
            Send(ds, event, RequestService::BATTALION, side, config_at);
            break;
          case RequestKind::REGIMENT:
            Send(ds, event, RequestService::REGIMENT, side, config_at);
            break;
          default: break;
        }
      }
      if (config.kind == std::to_underlying(RequestKind::REGIMENT))
        Send(ds, event, RequestService::COMPLETE, side);
      config.kind = std::to_underlying(RequestKind::NONE);
    }
    for (auto& request : data.sides[side].requests) {
      if (request.kind == std::to_underlying(RequestKind::NONE) ||
          RandomBelow(ds, 6) != 0)
        continue;
      switch (static_cast<RequestKind>(request.kind)) {
        case RequestKind::SUPPLIES:
          threshold = static_cast<uint16_t>(config.rate0 * 10);
          break;
        case RequestKind::BATTALION:
          threshold =
              opeu_percent(config.rate1 * 10,
                           data.request_weights1[request.argument] * 10) /
              10;
          break;
        case RequestKind::REGIMENT:
          threshold =
              opeu_percent(config.rate1 * 3,
                           data.request_weights2[request.argument] * 10) /
              10;
          break;
        default: break;
      }
      if (config.enabled[request.kind] == 1 &&
          config.kind == std::to_underlying(RequestKind::NONE) &&
          static_cast<uint8_t>(RandomBelow(ds, 1000)) < threshold)
        Send(ds, event, RequestService::START, side, Near(data, request));
      else
        Send(ds, event, RequestService::FALLBACK, side, Near(data, request));
      break;
    }
  }
}

uint16_t svc_requests(uint8_t* ds, uint16_t id, uint16_t a, uint16_t b,
                      uint16_t, uint16_t) {
  uint16_t r = 0;
  switch (static_cast<RequestService>(id)) {
    case RequestService::VALID:
      r = opeu_request_valid(ds, svc_reqvalid, (uint8_t)a, b);
      break;
    case RequestService::SUPPLIES:
      opeu_request_action0(ds, svc_reqres, (uint8_t)a, b);
      break;
    case RequestService::BATTALION:
      opeu_request_action1(ds, svc_reqequip, (uint8_t)a, b);
      break;
    case RequestService::REGIMENT:
      opeu_request_action2(ds, svc_reqspawn, (uint8_t)a, b);
      break;
    case RequestService::COMPLETE: opeu_request_complete(ds, (uint8_t)a); break;
    case RequestService::START:
      opeu_request_start(ds, svc_reqpend, (uint8_t)a, b);
      break;
    case RequestService::FALLBACK:
      opeu_request_fallback(ds, svc_reqpend, (uint8_t)a, b);
      break;
    default: port_unhandled("requests", id, 4);
  }
  return r;
}
