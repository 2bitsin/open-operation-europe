// Flashing map cells: an objective's or a unit's cell around a report, a division's units while its position is
// reported, an arriving unit, and a location shown to a player's side.
#include "map/mapunit.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "map/blast.hpp"
#include "map/mapcell.hpp"
#include "map/maperase.hpp"
#include "map/mappixel.hpp"
#include "map/mappos.hpp"
#include "types.hpp"

#include "port.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t  NO_SIDE{ 255 };
constexpr uint16_t MARKER_MODE{ 7 };
constexpr uint16_t FLASH_BUFFER_BYTES{ 32 }; // the background saved under each unit's marker
// The locals of opeu_group_show below its frame: the marker's pixel position and the division's selected unit, which
// stays as it was when no unit of the division is selected.
constexpr uint16_t PIXEL_X_AT{ 6 };
constexpr uint16_t PIXEL_Y_AT{ 10 };
constexpr uint16_t SELECTED_AT{ 14 };

[[nodiscard]] auto OverviewUp(DataSegment const& data) -> bool {
  return data.overview_side != NO_SIDE;
}

void Wait(uint8_t* ds, HostEvent event, uint16_t ticks) {
  Call(ds, event, MapService::WAIT, ticks);
}
} // namespace

// Flashes an active objective's cell twice around a delay. frame: this routine's frame.
void opeu_unit_flash(uint8_t* ds, HostEvent event, uint16_t frame, uint16_t record, uint8_t ticks) {
  auto const& data{ Data(ds) };
  auto const& objective{ Record<Objective>(data, record) };
  if (!OverviewUp(data) || !objective.active) return;
  opeu_map_cell_flash(ds, event, Local(frame, 10), objective.x, objective.y);
  Wait(ds, event, ticks);
  opeu_map_cell_flash(ds, event, Local(frame, 10), objective.x, objective.y);
}

// Flashes a visible unit's cell around the report of its regiment's position.
void opeu_unit_announce(uint8_t* ds, HostEvent event, uint16_t frame, uint16_t unit) {
  auto const& data{ Data(ds) };
  auto const& record{ Record<Unit>(data, unit) };
  if (!OverviewUp(data) || !(record.flags & Bits(UnitFlag::READY)) || !opeu_unit_visible(ds, unit)) return;
  opeu_map_cell_flash(ds, event, Local(frame, 6), record.x, record.y);
  Call(ds, event, MapService::CONTROL_MESSAGE, Near(data, data.regiment_position_text),
       opeu_find_group_record(ds, data.current_side, opeu_unit_group_index(unit)));
  opeu_map_cell_flash(ds, event, Local(frame, 6), record.x, record.y);
}

// Marks the division's visible units on the overview, reports the division's position at its selected unit (shown
// first), then restores the cells under the markers.
void opeu_group_show(uint8_t* ds, HostEvent event, uint16_t frame, uint16_t group, uint8_t person) {
  auto&      data{ Data(ds) };
  auto const first{ (opeu_group_side(group) * GROUPS_PER_SIDE + opeu_group_index(group)) * UNITS_PER_GROUP };
  auto const buffer{ [&](unsigned i) {
    return static_cast<uint16_t>(Near(data, data.flash_buffers) + i * FLASH_BUFFER_BYTES);
  } };
  auto const shown{ [&](Unit const& unit) {
    return OverviewUp(data) && (unit.flags & Bits(UnitFlag::READY)) && opeu_unit_visible(ds, Near(data, unit));
  } };
  auto const pixel_of{ [&](Unit const& unit) {
    return CellPixel(ds, Local(frame, PIXEL_X_AT), Local(frame, PIXEL_Y_AT), unit.x, unit.y, opeu_overview_pixel);
  } };
  for (unsigned i{ 0 }; i < UNITS_PER_GROUP; i++) {
    auto const& unit{ data.units[first + i] };
    if (!shown(unit)) continue;
    if (unit.flags & Bits(UnitFlag::SELECTED)) SetWord(ds, Local(frame, SELECTED_AT), Near(data, unit));
    auto const pixel{ pixel_of(unit) };
    Call(ds, event, MapService::SAVE_NEAR, pixel.x, pixel.y, 1, 8, buffer(i));
    Call(ds, event, MapService::DRAW_NEAR, pixel.x, pixel.y, 1, 8, MARKER_MODE, Near(data, data.flash_marker));
  }
  if (OverviewUp(data) && (Record<Group>(data, group).flags & Bits(GroupFlag::READY))) {
    auto const& selected{ Record<Unit>(data, Word(ds, Local(frame, SELECTED_AT))) };
    opeu_arrival_location(ds, event, Local(frame, 0x1c), selected.x, selected.y, data.current_side);
    auto const commander{ opeu_find_group_record(ds, data.current_side, person) };
    Call(ds, event, MapService::CONTROL_MESSAGE, Near(data, data.division_position_text), commander);
  }
  for (unsigned i{ 0 }; i < UNITS_PER_GROUP; i++) {
    auto const& unit{ data.units[first + i] };
    if (!shown(unit)) continue;
    auto const pixel{ pixel_of(unit) };
    Call(ds, event, MapService::RESTORE_NEAR, pixel.x, pixel.y, 1, 8, 0, buffer(i));
  }
}

// Blinks a visible arriving unit: removed, placed, removed, placed.
void opeu_arrival_view(uint8_t* ds, HostEvent event, uint16_t frame, uint16_t unit) {
  if (!opeu_unit_visible(ds, unit)) return;
  for (uint8_t blink{ 0 }; blink < 4; blink++) {
    if (blink & 1)
      opeu_arrival_prepare(ds, event, Local(frame, 10), unit);
    else
      opeu_unit_remove_draw(ds, event, Local(frame, 10), unit);
    Wait(ds, event, 2);
  }
}

// Six timed flashes of a cell for a player's side.
void opeu_side_cell_flash(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t x, uint8_t y, uint8_t side) {
  if (Data(ds).side_status[side] != SIDE_PLAYED) return;
  for (uint8_t flash{ 0 }; flash < 6; flash++) {
    opeu_map_cell_flash(ds, event, Local(frame, 12), x, y);
    Wait(ds, event, 2);
  }
}

// Shows the location, then flashes it twice over.
void opeu_location_flash(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t x, uint8_t y, uint8_t side) {
  opeu_arrival_location(ds, event, Local(frame, 14), x, y, side);
  for (uint8_t round{ 0 }; round < 2; round++) opeu_side_cell_flash(ds, event, Local(frame, 14), x, y, side);
}

uint16_t svc_mapunit(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count) {
  return svc_map_plan(ds, "mapunit", kind, args, count);
}

auto port_routine_opeu_unit_flash(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) {
    opeu_unit_flash(ds, svc_mapunit, BattleFrame(sp), Arg(args, count, 1), static_cast<uint8_t>(Arg(args, count, 0)));
  });
  return 0;
}
