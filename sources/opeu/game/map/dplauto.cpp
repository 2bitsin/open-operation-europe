// Automatic deployment. A unit's elements are placed by template (zone codes per approach direction), by the
// attacker's scan rectangles over the zone bits and clear terrain, at random inside the field centre, or, for a
// defending garrison, around the defenders already placed using the facing offsets.
#include "map/dplauto.hpp"
#include "common/battle-calls.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/dplcell.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {
// DS scratch for the cells these routines hand each other by near address.
constexpr uint16_t TEMPLATE_X_AT{ 0xeb80 };
constexpr uint16_t TEMPLATE_Y_AT{ 0xeb81 };
constexpr uint16_t AUTO_X_AT{ 0xeb82 };
constexpr uint16_t AUTO_Y_AT{ 0xeb83 };
constexpr uint16_t GARRISON_X_AT{ 0xeb84 };
constexpr uint16_t GARRISON_Y_AT{ 0xeb85 };
constexpr uint16_t SPARE_X_AT{ 0xeb86 };
constexpr uint16_t SPARE_Y_AT{ 0xeb87 };
// The approach direction of a unit that defends.
constexpr uint8_t DEFENDING{ 9 };
// Battle terrain: the kind in the low six bits; 1 is clear.
constexpr uint8_t TERRAIN_KIND{ 0x3f };
constexpr uint8_t CLEAR{ 1 };
// The field centre tried first at random.
constexpr uint8_t CENTRE_FIRST{ 5 };
constexpr uint8_t CENTRE_END{ 9 };
constexpr uint8_t SCAN_CELLS{ 4 };
constexpr uint8_t OFFSET_CELLS{ 5 };
constexpr uint8_t ZONE_CODES{ 8 };

[[nodiscard]] auto Random(Game& game, BattleEvent event, uint16_t bound) -> uint16_t {
  return Call(game, event, BattleRoutine::RAND_BOUNDED, bound);
}

[[nodiscard]] auto CanPlace(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element) -> bool {
  return opeu_deploy_can_place(game, event, x, y, element);
}

[[nodiscard]] auto ClearTerrain(DataSegment const& data, uint8_t x, uint8_t y) -> bool {
  return (data.battle_terrain[y * BATTLE_WIDTH + x] & TERRAIN_KIND) == CLEAR;
}

// A signed nibble: bit 3 the sign, bits 0..2 the magnitude.
[[nodiscard]] auto SignedNibble(uint8_t nibble) -> int {
  return nibble & 8 ? -(nibble & 7) : nibble;
}

// Scans the attacker's 4 x 4 rectangle row by row (directions 1 and 6) or column by column; a cell is accepted in
// the attacker's zone, on clear terrain (or, without `clear`, by a 3 in 10 chance) where the element may stand.
[[nodiscard]] auto ScanRectangle(Game& game, BattleEvent event, uint8_t direction, bool clear, uint16_t element, uint8_t& found_x, uint8_t& found_y) -> bool {
  auto& data{ Data(game) };
  // Read where the original reads it, for any direction.
  auto const scan{ [&](uint8_t n) { return Byte(game, static_cast<uint16_t>(Near(data, data.deploy_scan) + direction * SCAN_CELLS + n)); } };
  auto const step_x{ scan(0) };
  auto const step_y{ scan(1) };
  auto const accept{ [&](uint8_t x, uint8_t y) {
    return (data.deploy_zone_bits[y][x] & 1 << direction) && (clear ? ClearTerrain(data, x, y) : Random(game, event, 10) < 3) && CanPlace(game, event, x, y, element);
  } };
  auto const found{ [&](uint8_t x, uint8_t y) {
    found_x = x;
    found_y = y;
    return true;
  } };
  if (direction == 1 || direction == 6) {
    uint8_t y{ scan(3) };
    for (uint8_t row{ 0 }; row < SCAN_CELLS; y += step_y, ++row) {
      uint8_t x{ scan(2) };
      for (uint8_t column{ 0 }; column < SCAN_CELLS; x += step_x, ++column)
        if (accept(x, y)) return found(x, y);
    }
  } else {
    uint8_t x{ scan(2) };
    for (uint8_t column{ 0 }; column < SCAN_CELLS; x += step_x, ++column) {
      uint8_t y{ scan(3) };
      for (uint8_t row{ 0 }; row < SCAN_CELLS; y += step_y, ++row)
        if (accept(x, y)) return found(x, y);
    }
  }
  return false;
}
} // namespace

uint16_t opeu_deploy_template_cell(Game& game, BattleEvent event, uint16_t x, uint16_t y, uint16_t element, uint8_t index) {
  auto&      data{ Data(game) };
  auto const unit{ Call(game, event, BattleRoutine::BATTLE_ELEMENT_UNIT, element) };
  auto const direction{ UnitDirection(game, event, unit) };
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
  for (uint8_t row{ 0 }; row < BATTLE_WIDTH; ++row)
    for (uint8_t column{ 0 }; column < BATTLE_WIDTH; ++column)
      if (data.deploy_templates[row][column] == code && CanPlace(game, event, column, row, element)) {
        Byte(game, x) = column;
        Byte(game, y) = row;
        return 1;
      }
  return 0;
}

void opeu_deploy_by_template(Game& game, BattleEvent event, uint16_t x, uint16_t y, uint16_t element) {
  for (uint8_t index{ 0 }; index < ZONE_CODES; ++index)
    if (opeu_deploy_template_cell(game, event, TEMPLATE_X_AT, TEMPLATE_Y_AT, element, index)) {
      Byte(game, x) = Byte(game, TEMPLATE_X_AT);
      Byte(game, y) = Byte(game, TEMPLATE_Y_AT);
      break;
    }
}

void opeu_deploy_by_scan(Game& game, BattleEvent event, uint16_t x, uint16_t y, uint16_t element) {
  auto&      data{ Data(game) };
  auto const direction{ UnitDirection(game, event, data.battle_units[0]) };
  uint8_t    found_x;
  uint8_t    found_y;
  auto const place{ [&] {
    Byte(game, x) = found_x;
    Byte(game, y) = found_y;
  } };
  if (ScanRectangle(game, event, direction, true, element, found_x, found_y) || ScanRectangle(game, event, direction, false, element, found_x, found_y)) {
    place();
    return;
  }
  // Clear terrain anywhere by row, else any cell by column at a 1 in 10 chance, until one is found.
  auto const sweep{ [&] {
    for (found_y = 0; found_y < BATTLE_WIDTH; ++found_y)
      for (found_x = 0; found_x < BATTLE_WIDTH; ++found_x)
        if (ClearTerrain(data, found_x, found_y) && CanPlace(game, event, found_x, found_y, element)) return true;
    for (found_x = 0; found_x < BATTLE_WIDTH; ++found_x)
      for (found_y = 0; found_y < BATTLE_WIDTH; ++found_y)
        if (CanPlace(game, event, found_x, found_y, element) && Random(game, event, 10) < 1) return true;
    return false;
  } };
  while (!sweep()) {
  }
  place();
}

void opeu_deploy_at_random(Game& game, BattleEvent event, uint16_t x, uint16_t y, uint16_t element) {
  uint8_t    found_x;
  uint8_t    found_y;
  auto const centre{ [&] {
    for (found_y = CENTRE_FIRST; found_y < CENTRE_END; ++found_y)
      for (found_x = CENTRE_FIRST; found_x < CENTRE_END; ++found_x)
        if (Random(game, event, 10) < 2 && CanPlace(game, event, found_x, found_y, element)) return true;
    return false;
  } };
  auto const anywhere{ [&] {
    for (found_x = 0; found_x < BATTLE_WIDTH; ++found_x)
      for (found_y = 0; found_y < BATTLE_WIDTH; ++found_y)
        if (CanPlace(game, event, found_x, found_y, element) && Random(game, event, 10) < 1) return true;
    return false;
  } };
  if (!centre())
    while (!anywhere()) {
    }
  Byte(game, x) = found_x;
  Byte(game, y) = found_y;
}

void opeu_deploy_put_at(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element) {
  auto& data{ Data(game) };
  PositionOf(Data(game), element).goal_x = x;
  PositionOf(Data(game), element).goal_y = y;
  PositionOf(Data(game), element).state  = Bits(BattleState::DEFEND);
  Call(game, event, BattleRoutine::BATTLE_SET_ORDER, element, 0);
  auto facing{ opeu_deploy_facing(game, event, element) };
  if (!ElementTypeOf(data, element).category) facing = 0;
  Call(game, event, BattleRoutine::BATTLE_DRAW_ELEMENT_AT, x, y, element, facing);
}

void opeu_deploy_garrison(Game& game, BattleEvent event, uint16_t unit) {
  auto& data{ Data(game) };
  auto  chain{ [&] { return Elements(data, Record<Unit>(data, unit)); } };
  // The elements of type mode above 1 first: each placed at random (or where it stands) marks the garrison's centre.
  for (auto it{ chain().begin() }; it != std::default_sentinel; ++it) {
    auto const element{ it.Near() };
    if (ElementTypeOf(data, element).gun_range <= 1) continue;
    if (PositionOf(Data(game), element).x == BATTLE_UNPLACED) {
      opeu_deploy_at_random(game, event, GARRISON_X_AT, GARRISON_Y_AT, element);
      opeu_deploy_put_at(game, event, Byte(game, GARRISON_X_AT), Byte(game, GARRISON_Y_AT), element);
    } else {
      Byte(game, GARRISON_X_AT) = PositionOf(Data(game), element).x;
      Byte(game, GARRISON_Y_AT) = PositionOf(Data(game), element).y;
    }
  }
  // The others around it, facing the attacker.
  uint8_t facing{ 0 };
  if (auto const attacker{ Call(game, event, BattleRoutine::FIND_UNIT_AT, data.battle_attacker_x, data.battle_attacker_y) }) facing = static_cast<uint8_t>(7 - UnitDirection(game, event, attacker));
  for (auto it{ chain().begin() }; it != std::default_sentinel; ++it) {
    auto const element{ it.Near() };
    if (PositionOf(Data(game), element).x != BATTLE_UNPLACED || ElementTypeOf(data, element).gun_range != 1) continue;
    auto const placed{ [&] {
      for (uint8_t n{ 0 }; n < OFFSET_CELLS; ++n) {
        // Read where the original reads it: a facing past 7 (an attacker that defends) leaves the table.
        auto const offset{ Byte(game, static_cast<uint16_t>(Near(data, data.deploy_offsets) + facing * OFFSET_CELLS + n)) };
        auto const cell_y{ static_cast<int16_t>(SignedNibble(offset & 0xf) + Byte(game, GARRISON_Y_AT)) };
        auto const cell_x{ static_cast<int16_t>(SignedNibble(offset >> 4) + Byte(game, GARRISON_X_AT)) };
        if (cell_x >= 0 && cell_x < static_cast<int16_t>(BATTLE_WIDTH) && cell_y >= 0 && cell_y < static_cast<int16_t>(BATTLE_WIDTH)
            && CanPlace(game, event, static_cast<uint8_t>(cell_x), static_cast<uint8_t>(cell_y), element)) {
          opeu_deploy_put_at(game, event, static_cast<uint8_t>(cell_x), static_cast<uint8_t>(cell_y), element);
          return true;
        }
      }
      return false;
    } };
    if (!placed()) {
      opeu_deploy_at_random(game, event, SPARE_X_AT, SPARE_Y_AT, element);
      opeu_deploy_put_at(game, event, Byte(game, SPARE_X_AT), Byte(game, SPARE_Y_AT), element);
    }
  }
}

void opeu_deploy_auto(Game& game, BattleEvent event, uint16_t unit) {
  auto& data{ Data(game) };
  auto  chain{ [&] { return Elements(data, Record<Unit>(data, unit)); } };
  if (UnitDirection(game, event, unit) == DEFENDING)
    for (auto const& element : chain())
      if (data.element_types[element.type].gun_range > 1) {
        opeu_deploy_garrison(game, event, unit);
        return;
      }
  for (auto it{ chain().begin() }; it != std::default_sentinel; ++it) {
    auto const element{ it.Near() };
    if (PositionOf(Data(game), element).x != BATTLE_UNPLACED) continue;
    if (UnitDirection(game, event, unit) != DEFENDING) {
      opeu_deploy_by_template(game, event, AUTO_X_AT, AUTO_Y_AT, element);
    } else if (unit != Near(data, data.garrison_unit)) {
      // The unit's place in its group of eight.
      auto const index{ static_cast<uint8_t>(static_cast<uint8_t>(static_cast<int16_t>(unit - UNITS_AT) / static_cast<int16_t>(sizeof(Unit))) & 7) };
      if (Call(game, event, BattleRoutine::CATEGORY_6, index) || Call(game, event, BattleRoutine::CATEGORY_7, index))
        opeu_deploy_at_random(game, event, AUTO_X_AT, AUTO_Y_AT, element);
      else
        opeu_deploy_by_scan(game, event, AUTO_X_AT, AUTO_Y_AT, element);
    } else {
      opeu_deploy_by_scan(game, event, AUTO_X_AT, AUTO_Y_AT, element);
    }
    opeu_deploy_put_at(game, event, Byte(game, AUTO_X_AT), Byte(game, AUTO_Y_AT), element);
  }
}

// The deployment group's services (dplauto, dplcell, dplrows, dplside, dplunit): btl_reorder puts the argument words
// in source order into the order the routine's register convention takes them.
uint16_t svc_battle_deploy(Game& game, uint32_t address, const uint16_t* args, unsigned count) {
  static constexpr unsigned char unit_name[]   = { 1, 0 };          // (unit, buffer)
  static constexpr unsigned char list_dialog[] = { 1, 2, 0 };       // (window, limit, flags)
  static constexpr unsigned char set_order[]   = { 1, 0 };          // (element, order)
  static constexpr unsigned char grid_step[]   = { 1, 2, 0 };       // (x*, y*, direction)
  static constexpr unsigned char put_range[]   = { 1, 2, 0 };       // (s, start, count)
  static constexpr unsigned char columns[]     = { 0, 2, 1, 3, 4 }; // (column, first, rows, second, third)
  static constexpr unsigned char message[]     = { 1, 2, 0 };       // (text, side, value)
  switch (address) {
  case std::to_underlying(BattleRoutine::UNIT_NAME):                return btl_reorder(game, address, args, count, unit_name, 2);
  case std::to_underlying(BattleRoutine::LIST_DIALOG):              return btl_reorder(game, address, args, count, list_dialog, 3);
  case std::to_underlying(BattleRoutine::BATTLE_SET_ORDER):         return btl_reorder(game, address, args, count, set_order, 2);
  case std::to_underlying(BattleRoutine::GRID_STEP):                return btl_reorder(game, address, args, count, grid_step, 3);
  case std::to_underlying(BattleRoutine::TEXT_PUT_RANGE):           return btl_reorder(game, address, args, count, put_range, 3);
  case std::to_underlying(BattleRoutine::TABLE_COLUMNS):            return btl_reorder(game, address, args, count, columns, 5);
  case std::to_underlying(BattleRoutine::BATTLE_COMMANDER_MESSAGE): return btl_reorder(game, address, args, count, message, 3);
  default:                                                          return port_service(game, address, args, count);
  }
}

// Services of this module: the deployment group's.
uint16_t svc_dplauto(Game& game, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_deploy(game, service, args, count);
}
