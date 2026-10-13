// The special-forces reconnaissance of a city or a unit: on success the target is marked spotted for the side.
#include "core/msnrecon.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/mission.hpp"
#include "core/msnrept.hpp"
#include "core/strings.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include <cstddef>
#include <utility>

#include "common/game.hpp"
#include "core/msnkill.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

constexpr auto OFFICER{ std::to_underlying(StaffRole::OPERATIONS_CHIEF) };

// A city name longer than this starts on a new line of the report.
constexpr uint16_t ONE_LINE_NAME{ 11 };

constexpr auto At(std::size_t offset) -> uint16_t {
  return static_cast<uint16_t>(offset);
}

} // namespace

void opeu_mission_recon_objective(Game& game, HostEvent event, uint8_t side) {
  ServiceLocals const locals{ 0x32 };
  auto&               data{ Data(game) };
  auto const          text{ locals.At(0) };
  auto const          target{ data.sides[side].order.target };
  auto&               city{ Record<Objective>(data, target) };
  opeu_string_copy(game, text, At(offsetof(DataSegment, recon_objective_text)));
  if (opeu_string_length(game, target) > ONE_LINE_NAME) opeu_string_append(game, text, At(offsetof(DataSegment, recon_newline)));
  opeu_string_append(game, text, target);
  auto const roll{ Call(game, event, MISSION_RANDOM, 100) };
  if (opeu_officer_rating(game, side, OFFICER) <= roll) {
    opeu_mission_report_failure(game, event, side, text);
    return;
  }
  if (data.side_status[side] == SIDE_PLAYED) opeu_mission_report_success(game, event, side, text, city.x, city.y);
  city.flags |= Bits(ObjectiveFlag::MARKED);
  opeu_officer_gain(game, side, OFFICER);
}

void opeu_mission_recon_unit(Game& game, HostEvent event, uint8_t side) {
  ServiceLocals const locals{ 0x50 };
  auto&               data{ Data(game) };
  auto const          text{ locals.At(0x1e) };
  auto const          name{ locals.At(0) };
  auto const          target{ data.sides[side].order.target };
  auto&               unit{ Record<Unit>(data, target) };
  opeu_string_copy(game, text, At(offsetof(DataSegment, recon_unit_text)));
  opeu_string_append(game, text, Call(game, event, MISSION_UNIT_NAME, target, name));
  if (opeu_officer_rating(game, side, OFFICER) <= Call(game, event, MISSION_RANDOM, 100)) {
    opeu_mission_report_failure(game, event, side, text);
    return;
  }
  if (data.side_status[side] == SIDE_PLAYED) opeu_mission_report_success(game, event, side, text, unit.x, unit.y);
  for (auto& element : Elements(data, unit)) element.flags |= Bits(ElementFlag::SPOTTED);
  // The views redraw.
  data.view_side = data.overview_side  = 0xff;
  unit.flags                          |= Bits(UnitFlag::SPOTTED);
  opeu_officer_gain(game, side, OFFICER);
}

uint16_t svc_msnrecon(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_mission("msnrecon", game, id, args, count);
}

auto port_routine_opeu_mission_recon_objective(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_mission_recon_objective(game, svc_msnrecon, (uint8_t)Arg(args, count, 0));
  return 0;
}

auto port_routine_opeu_mission_recon_unit(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_mission_recon_unit(game, svc_msnrecon, (uint8_t)Arg(args, count, 0));
  return 0;
}
