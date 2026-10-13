// The hourly handling of the sides' requests: a booked request is carried out when its delay runs out, and each hour a
// filed request may reach headquarters, which accepts or turns it down.
#include "core/requests.hpp"
#include "common/random.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/template.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/reqcheck.hpp"
#include "core/reqequip.hpp"
#include "core/reqpend.hpp"
#include "core/reqres.hpp"
#include "core/reqspawn.hpp"
#include "core/reqvalid.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {

constexpr uint8_t TEMPLATE_KIND{ 5 }; // the person template that speaks the request reports

} // namespace

// For each side: the request booked in its configuration counts its delay down and, when it runs out, is carried out
// if it is still valid; then the first filed request that the hour's one-in-six roll picks reaches headquarters and is
// accepted when its kind is enabled, no other request is booked and a roll in 0..255 falls below the acceptance
// threshold of its kind. The threshold of an unknown kind is the caller's stale_threshold (the game reads an
// uninitialised word there).
void opeu_hourly_c_stage_b(Game& game, uint16_t stale_threshold) {
  auto& data{ Data(game) };
  auto  threshold{ stale_threshold };
  for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
    auto&      config{ data.request_configs[side] };
    auto const config_at{ Near(data, config) };
    data.request_template = opeu_find_person_template(game, Record<Person>(data, opeu_find_side_record(game, side, std::to_underlying(StaffRole::ARMY_COMMANDER))).status, TEMPLATE_KIND);
    data.request_person   = opeu_find_side_record(game, side, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF));
    if (config.kind != std::to_underlying(RequestKind::NONE) && opeu_sub_byte(1, &config.delay) == 0) {
      if (opeu_request_valid(game, side, config_at)) {
        switch (static_cast<RequestKind>(config.kind)) {
        case RequestKind::SUPPLIES:  opeu_request_action0(game, side, config_at); break;
        case RequestKind::BATTALION: opeu_request_action1(game, side, config_at); break;
        case RequestKind::REGIMENT:  opeu_request_action2(game, side, config_at); break;
        default:                     break;
        }
      }
      if (config.kind == std::to_underlying(RequestKind::REGIMENT)) opeu_request_complete(game, side);
      config.kind = std::to_underlying(RequestKind::NONE);
    }
    for (auto& request : data.sides[side].requests) {
      if (request.kind == std::to_underlying(RequestKind::NONE) || RandomBelow(game, 6) != 0) continue;
      switch (static_cast<RequestKind>(request.kind)) {
      case RequestKind::SUPPLIES:  threshold = static_cast<uint16_t>(config.supply_rate * 10); break;
      case RequestKind::BATTALION: threshold = opeu_percent(config.reinforcement_rate * 10, data.type_availability[request.argument] * 10) / 10; break;
      case RequestKind::REGIMENT:  threshold = opeu_percent(config.reinforcement_rate * 3, data.regiment_kind_availability[request.argument] * 10) / 10; break;
      default:                     break;
      }
      if (config.enabled[request.kind] == 1 && config.kind == std::to_underlying(RequestKind::NONE) && static_cast<uint8_t>(RandomBelow(game, 1000)) < threshold)
        opeu_request_start(game, side, Near(data, request));
      else
        opeu_request_fallback(game, side, Near(data, request));
      break;
    }
  }
}
