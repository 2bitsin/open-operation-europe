// The special-forces reconnaissance of a city or a unit: on success the target is marked spotted for the side.
#include "types.hpp"
#include "core/msnrecon.hpp"
#include "core/msnrept.hpp"
#include "core/strings.hpp"
#include "common/events.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "common/mission.hpp"
#include "data-segment.hpp"

#include <cstddef>
#include <utility>

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "core/msnkill.hpp"

using namespace opeu::game;

namespace {

constexpr auto OFFICER{ std::to_underlying(StaffRole::OPERATIONS_CHIEF) };

// A city name longer than this starts on a new line of the report.
constexpr uint16_t ONE_LINE_NAME{ 11 };

constexpr auto At(std::size_t offset) -> uint16_t {
  return static_cast<uint16_t>(offset);
}

} // namespace

void opeu_mission_recon_objective(uint8_t ds[65536], HostEvent event,
                                  uint16_t frame, uint8_t side) {
  auto&      data{ Data(ds) };
  auto const text{ Local(frame, 50) };
  auto const target{ data.sides[side].order.target };
  auto&      city{ Record<Objective>(data, target) };
  opeu_string_copy(ds, text, At(offsetof(DataSegment, recon_objective_text)));
  if (opeu_string_length(ds, target) > ONE_LINE_NAME)
    opeu_string_append(ds, text, At(offsetof(DataSegment, recon_newline)));
  opeu_string_append(ds, text, target);
  auto const roll{ Call(ds, event, MISSION_RANDOM, 100) };
  if (opeu_officer_rating(ds, side, OFFICER) <= roll) {
    opeu_mission_report_failure(ds, event, frame, side, text);
    return;
  }
  if (data.side_status[side] == SIDE_PLAYED)
    opeu_mission_report_success(ds, event, frame, side, text, city.x, city.y);
  city.flags |= Bits(ObjectiveFlag::MARKED);
  opeu_officer_gain(ds, side, OFFICER);
}

void opeu_mission_recon_unit(uint8_t ds[65536], HostEvent event, uint16_t frame,
                             uint8_t side) {
  auto&      data{ Data(ds) };
  auto const text{ Local(frame, 50) };
  auto const name{ Local(frame, 80) };
  auto const target{ data.sides[side].order.target };
  auto&      unit{ Record<Unit>(data, target) };
  opeu_string_copy(ds, text, At(offsetof(DataSegment, recon_unit_text)));
  opeu_string_append(ds, text,
                     Call(ds, event, MISSION_UNIT_NAME, target, name));
  if (opeu_officer_rating(ds, side, OFFICER) <=
      Call(ds, event, MISSION_RANDOM, 100)) {
    opeu_mission_report_failure(ds, event, frame, side, text);
    return;
  }
  if (data.side_status[side] == SIDE_PLAYED)
    opeu_mission_report_success(ds, event, frame, side, text, unit.x, unit.y);
  for (auto& element : Elements(data, unit))
    element.flags |= Bits(ElementFlag::SPOTTED);
  // The views redraw.
  data.view_side = data.overview_side = 0xff;
  unit.flags |= Bits(UnitFlag::SPOTTED);
  opeu_officer_gain(ds, side, OFFICER);
}

uint16_t svc_msnrecon(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count) {
  return svc_mission("msnrecon", ds, id, args, count);
}

auto port_routine_opeu_mission_recon_objective(std::uint8_t*        ds,
                                               std::uint16_t const* args,
                                               unsigned             count)
    -> std::uint32_t {
  uint16_t frame = port_frame_enter();
  opeu_mission_recon_objective(ds, svc_msnrecon, frame,
                               (uint8_t)Arg(args, count, 0));
  port_frame_leave();
  return 0;
}

auto port_routine_opeu_mission_recon_unit(std::uint8_t*        ds,
                                          std::uint16_t const* args,
                                          unsigned count) -> std::uint32_t {
  uint16_t frame = port_frame_enter();
  opeu_mission_recon_unit(ds, svc_msnrecon, frame,
                          (uint8_t)Arg(args, count, 0));
  port_frame_leave();
  return 0;
}
