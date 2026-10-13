// Flashing map cells: an objective's or a unit's cell around a report, a division's units while its position is
// reported, an arriving unit, and a location shown to a player's side.
#include "map/mapunit.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/blast.hpp"
#include "map/mapcell.hpp"
#include "map/maperase.hpp"
#include "map/mappixel.hpp"
#include "map/mappos.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "vgalib.hpp"

#include <ranges>

using namespace opeu::game;

namespace {
constexpr uint16_t MARKER_MODE{ 7 };
constexpr uint16_t FLASH_BUFFER_BYTES{ 32 }; // the background saved under each unit's marker

[[nodiscard]] auto OverviewUp(DataSegment const& data) -> bool {
  return data.overview_side != NO_SIDE;
}

void Wait(Game& game, uint16_t ticks) {
  opeu_delay_units(game, static_cast<uint8_t>(ticks));
}
} // namespace

// Flashes an active objective's cell twice around a delay. frame: this routine's frame.
void opeu_unit_flash(Game& game, uint16_t record, uint8_t ticks) {
  auto const& data{ Data(game) };
  auto const& objective{ Record<Objective>(data, record) };
  if (!OverviewUp(data) || !objective.active) return;
  opeu_map_cell_flash(game, objective.x, objective.y);
  Wait(game, ticks);
  opeu_map_cell_flash(game, objective.x, objective.y);
}

// Flashes a visible unit's cell around the report of its regiment's position.
void opeu_unit_announce(Game& game, uint16_t unit) {
  auto const& data{ Data(game) };
  auto const& record{ Record<Unit>(data, unit) };
  if (!OverviewUp(data) || !(record.flags & Bits(UnitFlag::READY)) || !opeu_unit_visible(game, unit)) return;
  opeu_map_cell_flash(game, record.x, record.y);
  opeu_control_message(game, static_cast<DialogHost const*>(port_dialogs_host()), static_cast<std::uint16_t>(Near(data, data.regiment_position_text)),
                       static_cast<std::uint16_t>(opeu_find_group_record(game, data.current_side, opeu_unit_group_index(unit))));
  opeu_map_cell_flash(game, record.x, record.y);
}

// Marks the division's visible units on the overview, reports the division's position at its selected unit (shown
// first), then restores the cells under the markers.
void opeu_group_show(Game& game, uint16_t group, uint8_t person) {
  auto&      data{ Data(game) };
  auto const first{ (opeu_group_side(group) * GROUPS_PER_SIDE + opeu_group_index(group)) * UNITS_PER_GROUP };
  auto const buffer{ [&](unsigned i) { return static_cast<uint16_t>(Near(data, data.flash_buffers) + i * FLASH_BUFFER_BYTES); } };
  auto const shown{ [&](Unit const& unit) { return OverviewUp(data) && (unit.flags & Bits(UnitFlag::READY)) && opeu_unit_visible(game, Near(data, unit)); } };
  auto const pixel_of{ [](Unit const& unit) { return OverviewPixel({ unit.x, unit.y }); } };
  uint16_t   selected_unit{ 0 }; // stays 0 when no shown unit of the division is selected
  for (auto const i : std::views::iota(unsigned{ 0 }, UNITS_PER_GROUP)) {
    auto const& unit{ data.units[first + i] };
    if (!shown(unit)) continue;
    if (unit.flags & Bits(UnitFlag::SELECTED)) selected_unit = Near(data, unit);
    auto const pixel{ pixel_of(unit) };
    koei_2e72(static_cast<std::uint16_t>(pixel.x), static_cast<std::uint16_t>(pixel.y), 1, 8, game.State().Bytes(static_cast<std::uint16_t>(buffer(i))).data());
    opeu_draw_near(static_cast<std::uint16_t>(pixel.x), static_cast<std::uint16_t>(pixel.y), 1, 8, MARKER_MODE, game.State().Bytes(static_cast<std::uint16_t>(Near(data, data.flash_marker))).data());
  }
  if (OverviewUp(data) && (Record<Group>(data, group).flags & Bits(GroupFlag::READY))) {
    auto const& selected{ Record<Unit>(data, selected_unit) };
    opeu_arrival_location(game, selected.x, selected.y, data.current_side);
    auto const commander{ opeu_find_group_record(game, data.current_side, person) };
    opeu_control_message(game, static_cast<DialogHost const*>(port_dialogs_host()), static_cast<std::uint16_t>(Near(data, data.division_position_text)), static_cast<std::uint16_t>(commander));
  }
  for (auto const i : std::views::iota(unsigned{ 0 }, UNITS_PER_GROUP)) {
    auto const& unit{ data.units[first + i] };
    if (!shown(unit)) continue;
    auto const pixel{ pixel_of(unit) };
    koei_266c(static_cast<std::uint16_t>(pixel.x), static_cast<std::uint16_t>(pixel.y), 1, 8, 0, game.State().Bytes(static_cast<std::uint16_t>(buffer(i))).data());
  }
}

// Blinks a visible arriving unit: removed, placed, removed, placed.
void opeu_arrival_view(Game& game, uint16_t unit) {
  if (!opeu_unit_visible(game, unit)) return;
  for (auto const blink : std::views::iota(uint8_t{ 0 }, uint8_t{ 4 })) {
    if (blink & 1)
      opeu_arrival_prepare(game, unit);
    else
      opeu_unit_remove_draw(game, unit);
    Wait(game, 2);
  }
}

// Six timed flashes of a cell for a player's side.
void opeu_side_cell_flash(Game& game, uint8_t x, uint8_t y, uint8_t side) {
  if (Data(game).side_status[side] != SIDE_PLAYED) return;
  for ([[maybe_unused]] auto const flash : std::views::iota(uint8_t{ 0 }, uint8_t{ 6 })) {
    opeu_map_cell_flash(game, x, y);
    Wait(game, 2);
  }
}

// Shows the location, then flashes it twice over.
void opeu_location_flash(Game& game, uint8_t x, uint8_t y, uint8_t side) {
  opeu_arrival_location(game, x, y, side);
  for ([[maybe_unused]] auto const round : std::views::iota(uint8_t{ 0 }, uint8_t{ 2 })) opeu_side_cell_flash(game, x, y, side);
}
