// Automatic deployment. A unit's elements are placed by template (zone codes per approach direction), by the
// attacker's scan rectangles over the zone bits and clear terrain, at random inside the field centre, or, for a
// defending garrison, around the defenders already placed using the facing offsets.
#include "map/dplauto.hpp"
#include "common/battle-calls.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlelem.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "map/dplcell.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <utility>

using namespace opeu::game;

namespace {
// DS scratch that keeps the last cell found for automatic placement and the garrison's centre between calls: a search
// that finds nothing leaves the previous cell in place.
struct ScratchCell {
  uint16_t x_at;
  uint16_t y_at;
};
constexpr ScratchCell AUTO_CELL{ 0xeb82, 0xeb83 };
constexpr ScratchCell GARRISON_CELL{ 0xeb84, 0xeb85 };
constexpr ScratchCell SPARE_CELL{ 0xeb86, 0xeb87 };
// The approach direction of a unit that defends.
constexpr uint8_t DEFENDING{ 9 };
// Battle terrain: the kind in the low six bits; 1 is clear.
constexpr uint8_t TERRAIN_KIND{ 0x3f };
constexpr uint8_t CLEAR{ 1 };
// The field centre tried first at random.
constexpr uint8_t CENTRE_FIRST{ 5 };
constexpr uint8_t CENTRE_END{ 9 };
constexpr uint8_t FIELD_END{ BATTLE_WIDTH };
constexpr uint8_t SCAN_CELLS{ 4 };
constexpr uint8_t OFFSET_CELLS{ 5 };
constexpr uint8_t ZONE_CODES{ 8 };

// What a scan rectangle accepts beside the attacker's zone and a free cell.
enum class Ground {
  CLEAR,     // clear terrain
  BY_CHANCE, // any terrain, by a 3 in 10 chance
};

[[nodiscard]] auto Load(Game& game, ScratchCell scratch) -> GridCell {
  return { Byte(game, scratch.x_at), Byte(game, scratch.y_at) };
}

void Store(Game& game, ScratchCell scratch, GridCell cell) {
  Byte(game, scratch.x_at) = cell.x;
  Byte(game, scratch.y_at) = cell.y;
}

[[nodiscard]] auto Random(Game& game, uint16_t bound) -> uint16_t {
  return game.State().RandomBounded(bound);
}

[[nodiscard]] auto CanPlace(Game& game, GridCell cell, uint16_t element) -> bool {
  return opeu_deploy_can_place(game, cell, element);
}

[[nodiscard]] auto ClearTerrain(DataSegment const& data, GridCell cell) -> bool {
  return (data.battle_terrain[cell.y * BATTLE_WIDTH + cell.x] & TERRAIN_KIND) == CLEAR;
}

// A signed nibble: bit 3 the sign, bits 0..2 the magnitude.
[[nodiscard]] auto SignedNibble(uint8_t nibble) -> int {
  return nibble & 8 ? -(nibble & 7) : nibble;
}

// The first cell of the square [first, end) x [first, end), row by row, that the predicate accepts.
template <typename _Accept>
[[nodiscard]] auto FirstByRows(uint8_t first, uint8_t end, _Accept const& accept) -> std::optional<GridCell> {
  for (auto const y : std::views::iota(first, end))
    for (auto const x : std::views::iota(first, end))
      if (accept(GridCell{ x, y })) return GridCell{ x, y };
  return std::nullopt;
}

// The same, column by column.
template <typename _Accept>
[[nodiscard]] auto FirstByColumns(uint8_t first, uint8_t end, _Accept const& accept) -> std::optional<GridCell> {
  for (auto const x : std::views::iota(first, end))
    for (auto const y : std::views::iota(first, end))
      if (accept(GridCell{ x, y })) return GridCell{ x, y };
  return std::nullopt;
}

// Scans the attacker's 4 x 4 rectangle for the approach direction, row by row (directions 1 and 6) or column by
// column; a cell is accepted in the attacker's zone, on the ground asked for, where the element may stand.
class ScanRectangle {
public:
  ScanRectangle(Game& game, uint8_t direction, uint16_t element) : _game{ game }, _direction{ direction }, _element{ element } { }

  [[nodiscard]] auto Find(Ground ground) const -> std::optional<GridCell> {
    auto const accept{ [&](GridCell offset) {
      auto const cell{ At(offset) };
      return Zoned(cell) && (ground == Ground::CLEAR ? ClearTerrain(Data(_game), cell) : Random(_game, 10) < 3) && CanPlace(_game, cell, _element);
    } };
    auto const offset{ _direction == 1 || _direction == 6 ? FirstByRows(0, SCAN_CELLS, accept) : FirstByColumns(0, SCAN_CELLS, accept) };
    if (!offset) return std::nullopt;
    return At(*offset);
  }

private:
  // The rectangle's table entry n, read where the table lies for any direction.
  [[nodiscard]] auto Scan(uint8_t n) const -> uint8_t {
    auto const& data{ Data(_game) };
    return Byte(_game, static_cast<uint16_t>(Near(data, data.deploy_scan) + _direction * SCAN_CELLS + n));
  }

  // The cell `offset` steps into the rectangle; byte arithmetic wraps.
  [[nodiscard]] auto At(GridCell offset) const -> GridCell {
    return { static_cast<uint8_t>(Scan(2) + offset.x * Scan(0)), static_cast<uint8_t>(Scan(3) + offset.y * Scan(1)) };
  }

  [[nodiscard]] auto Zoned(GridCell cell) const -> bool {
    return Data(_game).deploy_zone_bits[cell.y][cell.x] & 1 << _direction;
  }

  Game&    _game;
  uint8_t  _direction;
  uint16_t _element;
};

// Places the element where it stands or, when it is not on the field, at a random cell, which becomes the garrison's
// centre.
void PlaceHeavy(Game& game, uint16_t element) {
  auto const& position{ PositionOf(Data(game), element) };
  if (position.x != BATTLE_UNPLACED) {
    Store(game, GARRISON_CELL, { position.x, position.y });
    return;
  }
  Store(game, GARRISON_CELL, opeu_deploy_at_random(game, element));
  opeu_deploy_put_at(game, element, Load(game, GARRISON_CELL));
}

// Places the element at the first free facing offset around the garrison's centre; false when none is free.
[[nodiscard]] auto PlaceAroundCentre(Game& game, uint16_t element, uint8_t facing) -> bool {
  auto&      data{ Data(game) };
  auto const centre{ Load(game, GARRISON_CELL) };
  for (auto const n : std::views::iota(uint8_t{ 0 }, OFFSET_CELLS)) {
    // Read where the table lies: a facing past 7 (an attacker that defends) leaves it.
    auto const offset{ Byte(game, static_cast<uint16_t>(Near(data, data.deploy_offsets) + facing * OFFSET_CELLS + n)) };
    auto const cell_y{ static_cast<int16_t>(SignedNibble(offset & 0xf) + centre.y) };
    auto const cell_x{ static_cast<int16_t>(SignedNibble(offset >> 4) + centre.x) };
    if (cell_x < 0 || cell_x >= static_cast<int16_t>(BATTLE_WIDTH) || cell_y < 0 || cell_y >= static_cast<int16_t>(BATTLE_WIDTH)) continue;
    GridCell const cell{ static_cast<uint8_t>(cell_x), static_cast<uint8_t>(cell_y) };
    if (!CanPlace(game, cell, element)) continue;
    opeu_deploy_put_at(game, element, cell);
    return true;
  }
  return false;
}

// The automatic cell of an element whose unit defends outside the garrison: at random for the categories 6 and 7 by
// the unit's place in its group of eight, otherwise by scan.
[[nodiscard]] auto DefenderCell(Game& game, uint16_t unit, uint16_t element) -> GridCell {
  auto const index{ static_cast<uint8_t>(static_cast<uint8_t>(static_cast<int16_t>(unit - UNITS_AT) / static_cast<int16_t>(sizeof(Unit))) & 7) };
  if (opeu_category_6(index) || opeu_category_7(index)) return opeu_deploy_at_random(game, element);
  return opeu_deploy_by_scan(game, element);
}

// Finds the element's automatic cell; the scratch keeps the previous one when the template has none.
void FindAutoCell(Game& game, uint16_t unit, uint16_t element) {
  auto& data{ Data(game) };
  if (UnitDirection(game, unit) != DEFENDING) {
    if (auto const cell{ opeu_deploy_by_template(game, element) }) Store(game, AUTO_CELL, *cell);
  } else if (unit != Near(data, data.garrison_unit)) {
    Store(game, AUTO_CELL, DefenderCell(game, unit, element));
  } else {
    Store(game, AUTO_CELL, opeu_deploy_by_scan(game, element));
  }
}
} // namespace

auto opeu_deploy_template_cell(Game& game, uint16_t element, uint8_t index) -> std::optional<GridCell> {
  auto&      data{ Data(game) };
  auto const unit{ opeu_battle_element_unit(game, element) };
  auto const direction{ UnitDirection(game, unit) };
  auto const heavy{ ElementTypeOf(data, element).gun_range > 1 };
  uint8_t    code{ 0 };
  switch (direction) {
  case 1:
  case 3:
  case 4:
  case 6: code = static_cast<uint8_t>(direction << 4 | data.deploy_zone_codes[heavy ? 0 : 2][index]); break;
  case 0:
  case 2:
  case 5:
  case 7: code = static_cast<uint8_t>(direction << 4 | data.deploy_zone_codes[heavy ? 1 : 3][index]); break;
  }
  return FirstByRows(0, FIELD_END, [&](GridCell cell) { return data.deploy_templates[cell.y][cell.x] == code && CanPlace(game, cell, element); });
}

auto opeu_deploy_by_template(Game& game, uint16_t element) -> std::optional<GridCell> {
  for (auto const index : std::views::iota(uint8_t{ 0 }, ZONE_CODES))
    if (auto const cell{ opeu_deploy_template_cell(game, element, index) }) return cell;
  return std::nullopt;
}

auto opeu_deploy_by_scan(Game& game, uint16_t element) -> GridCell {
  auto&               data{ Data(game) };
  ScanRectangle const rectangle{ game, UnitDirection(game, data.battle_units[0]), element };
  if (auto const cell{ rectangle.Find(Ground::CLEAR) }) return *cell;
  if (auto const cell{ rectangle.Find(Ground::BY_CHANCE) }) return *cell;
  // Clear terrain anywhere by row, else any cell by column at a 1 in 10 chance, until one is found.
  auto const sweep{ [&] {
    if (auto const cell{ FirstByRows(0, FIELD_END, [&](GridCell cell) { return ClearTerrain(data, cell) && CanPlace(game, cell, element); }) }) return cell;
    return FirstByColumns(0, FIELD_END, [&](GridCell cell) { return CanPlace(game, cell, element) && Random(game, 10) < 1; });
  } };
  for (;;)
    if (auto const cell{ sweep() }) return *cell;
}

auto opeu_deploy_at_random(Game& game, uint16_t element) -> GridCell {
  if (auto const cell{ FirstByRows(CENTRE_FIRST, CENTRE_END, [&](GridCell cell) { return Random(game, 10) < 2 && CanPlace(game, cell, element); }) }) return *cell;
  for (;;)
    if (auto const cell{ FirstByColumns(0, FIELD_END, [&](GridCell cell) { return CanPlace(game, cell, element) && Random(game, 10) < 1; }) }) return *cell;
}

void opeu_deploy_put_at(Game& game, uint16_t element, GridCell cell) {
  auto& data{ Data(game) };
  auto& position{ PositionOf(data, element) };
  position.goal_x = cell.x;
  position.goal_y = cell.y;
  position.state  = Bits(BattleState::DEFEND);
  opeu_battle_set_order(game, element, 0);
  auto facing{ opeu_deploy_facing(game, element) };
  if (!ElementTypeOf(data, element).category) facing = 0;
  opeu_battle_draw_element_at(game, cell, { element, facing });
}

void opeu_deploy_garrison(Game& game, uint16_t unit) {
  auto&      data{ Data(game) };
  auto const chain{ [&] { return Elements(data, Record<Unit>(data, unit)); } };
  // The elements of type mode above 1 first: each placed at random (or where it stands) marks the garrison's centre.
  for (auto it{ chain().begin() }; it != std::default_sentinel; ++it)
    if (ElementTypeOf(data, it.Near()).gun_range > 1) PlaceHeavy(game, it.Near());
  // The others around it, facing the attacker.
  uint8_t facing{ 0 };
  if (auto const attacker{ opeu_find_unit_at(game, data.battle_attacker_x, data.battle_attacker_y) }) facing = static_cast<uint8_t>(7 - UnitDirection(game, attacker));
  for (auto it{ chain().begin() }; it != std::default_sentinel; ++it) {
    auto const element{ it.Near() };
    if (PositionOf(data, element).x != BATTLE_UNPLACED || ElementTypeOf(data, element).gun_range != 1 || PlaceAroundCentre(game, element, facing)) continue;
    Store(game, SPARE_CELL, opeu_deploy_at_random(game, element));
    opeu_deploy_put_at(game, element, Load(game, SPARE_CELL));
  }
}

void opeu_deploy_auto(Game& game, uint16_t unit) {
  auto&      data{ Data(game) };
  auto const chain{ [&] { return Elements(data, Record<Unit>(data, unit)); } };
  auto const heavy{ [&](Element const& element) { return data.element_types[element.type].gun_range > 1; } };
  if (UnitDirection(game, unit) == DEFENDING && std::ranges::any_of(chain(), heavy)) {
    opeu_deploy_garrison(game, unit);
    return;
  }
  for (auto it{ chain().begin() }; it != std::default_sentinel; ++it) {
    auto const element{ it.Near() };
    if (PositionOf(data, element).x != BATTLE_UNPLACED) continue;
    FindAutoCell(game, unit, element);
    opeu_deploy_put_at(game, element, Load(game, AUTO_CELL));
  }
}
