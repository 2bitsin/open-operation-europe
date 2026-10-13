// Battle path finding and enemy spotting.
//
// The path cost of a cell is its terrain's cost for the element's category (255 when occupied, unless occupants are
// ignored). The flood fills path costs from a cell over the battle grid, with a queue kept sorted by cost, until it
// reaches the other cell or runs out; the path step then moves a cell one step downhill in those costs. Spotting
// marks the enemy elements within the element's range whose chance roll (terrain mode, cover, weather) succeeds,
// and reports each to the player.
#include "map/btlpath.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/memory.hpp"
#include "core/numeric.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "types.hpp"
#include "units/labels.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t  BLOCKED{ 0xff };
constexpr uint16_t UNREACHED{ 0xffff };
constexpr uint8_t  DIRECTIONS{ 4 };
constexpr uint8_t  CELLS{ BATTLE_WIDTH * BATTLE_WIDTH };
// The flood's mode bits: stop on reaching the other cell, ignore occupants.
constexpr uint8_t FLOOD_TO_CELL{ 1 };
constexpr uint8_t FLOOD_THROUGH{ 2 };
// The DS reserves of the locals the battle services receive by address, and their offsets in them: path_step's
// neighbour under test and the two bytes it keeps when no neighbour is reachable (never written: whatever the reserve
// holds); flood's neighbour under test; report_sighted's name and message buffers.
constexpr uint16_t STEP_RESERVE{ 0x0a };
constexpr uint16_t STEP_X_AT{ 6 };
constexpr uint16_t STEP_Y_AT{ 4 };
constexpr uint16_t STEP_STALE_X_AT{ 8 };
constexpr uint16_t STEP_STALE_Y_AT{ 0 };
constexpr uint16_t FLOOD_RESERVE{ 8 };
constexpr uint16_t FLOOD_X_AT{ 6 };
constexpr uint16_t FLOOD_Y_AT{ 0 };
constexpr uint16_t SIGHTED_RESERVE{ 0x86 };
constexpr uint16_t NAME_AT{ 0 };
constexpr uint16_t MESSAGE_AT{ 0x20 };
// Spotting: the chance per terrain mode step, the cover of forest, bridge and town cells, and the weather's toll.
constexpr uint8_t SPOT_STEPS{ 6 };
constexpr uint8_t SPOT_STEP{ 20 };

[[nodiscard]] auto TerrainCost(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t element) -> uint8_t {
  auto const& data{ Data(ds) };
  auto const  terrain{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_CELL_TERRAIN, x, y)) };
  return data.battle_terrain_costs[terrain * 8u + ElementTypeOf(data, element).category];
}

[[nodiscard]] auto Cover(uint8_t terrain) -> uint8_t {
  switch (terrain) {
  case 9:  return 20;
  case 10: return 10;
  case 6:  return 30;
  default: return 0;
  }
}

[[nodiscard]] auto WeatherToll(uint8_t weather) -> uint8_t {
  switch (weather) {
  case 2:  return 10;
  case 3:  return 15;
  case 4:  return 30;
  case 5:  return 25;
  default: return 0;
  }
}
} // namespace

uint8_t opeu_battle_path_cost(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t element) {
  if (Call(ds, event, BattleRoutine::BATTLE_CELL_OCCUPIED, x, y)) return BLOCKED;
  return TerrainCost(ds, event, x, y, element);
}

uint8_t opeu_battle_path_cost_any(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t element) {
  return TerrainCost(ds, event, x, y, element);
}

// x, y: DS offsets of the cell bytes, moved to the cheapest neighbour. With no reachable neighbour the cell takes the
// reserve's stale bytes, as the original's uninitialized locals.
uint16_t opeu_battle_path_step(uint8_t* ds, BattleEvent event, uint16_t x, uint16_t y) {
  auto const&         data{ Data(ds) };
  ServiceLocals const locals{ STEP_RESERVE };
  auto const          step_x_at{ locals.At(STEP_X_AT) };
  auto const          step_y_at{ locals.At(STEP_Y_AT) };
  uint16_t            best{ UNREACHED };
  uint8_t             best_x{ 0 };
  uint8_t             best_y{ 0 };
  for (uint8_t direction{ 0 }; direction < DIRECTIONS; ++direction) {
    Byte(ds, step_x_at) = Byte(ds, x);
    Byte(ds, step_y_at) = Byte(ds, y);
    if (!Call(ds, event, BattleRoutine::BATTLE_STEP, step_y_at, direction, step_x_at)) continue;
    auto const cost{ data.battle_path_costs[Byte(ds, step_y_at) * BATTLE_WIDTH + Byte(ds, step_x_at)] };
    if (cost < best) {
      best_x = Byte(ds, step_x_at);
      best_y = Byte(ds, step_y_at);
      best   = cost;
    }
  }
  if (best == UNREACHED) {
    best_x = Byte(ds, locals.At(STEP_STALE_X_AT));
    best_y = Byte(ds, locals.At(STEP_STALE_Y_AT));
  }
  Byte(ds, x) = best_x;
  Byte(ds, y) = best_y;
  return best;
}

// Fills battle_path_costs from (x, y). Returns 1 when the flood reached (to_x, to_y) with FLOOD_TO_CELL set.
uint16_t opeu_battle_path_flood(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint8_t to_x, uint8_t to_y, uint16_t element, uint8_t mode) {
  auto&               data{ Data(ds) };
  auto&               queue{ data.battle_path_queue };
  auto&               costs{ data.battle_path_costs };
  ServiceLocals const locals{ FLOOD_RESERVE };
  auto const          step_x_at{ locals.At(FLOOD_X_AT) };
  auto const          step_y_at{ locals.At(FLOOD_Y_AT) };
  unsigned            head{ 0 };
  unsigned            tail{ 1 };
  queue[head] = static_cast<uint8_t>(x + y * BATTLE_WIDTH);
  opeu_fill_bytes(ds, Near(data, costs), CELLS * 2, BLOCKED);
  costs[queue[0]] = 0;
  while (tail != head) {
    auto const from_x{ static_cast<uint8_t>(queue[head] % BATTLE_WIDTH) };
    auto const from_y{ static_cast<uint8_t>(queue[head] / BATTLE_WIDTH) };
    auto const cost{ costs[from_y * BATTLE_WIDTH + from_x] };
    for (uint8_t direction{ 0 }; direction < DIRECTIONS; ++direction) {
      Byte(ds, step_x_at) = from_x;
      Byte(ds, step_y_at) = from_y;
      if (!Call(ds, event, BattleRoutine::BATTLE_STEP, step_y_at, direction, step_x_at)) continue;
      auto const step_x{ Byte(ds, step_x_at) };
      auto const step_y{ Byte(ds, step_y_at) };
      if (to_x == step_x && to_y == step_y && (mode & FLOOD_TO_CELL)) return 1;
      auto const cell{ static_cast<uint8_t>(step_y * BATTLE_WIDTH + step_x) };
      if (costs[cell] != UNREACHED) continue;
      auto const entry{ mode & FLOOD_THROUGH ? opeu_battle_path_cost_any(ds, event, step_x, step_y, element) : opeu_battle_path_cost(ds, event, step_x, step_y, element) };
      if (entry == BLOCKED) continue;
      costs[cell] = static_cast<uint16_t>(entry + cost);
      // Insert the cell after the last entry no dearer than it (the start cell costs 0, so the walk stops there).
      for (auto at{ tail };; --at) {
        queue[at] = queue[at - 1];
        if (costs[queue[at - 1]] <= costs[cell]) {
          queue[at] = cell;
          ++tail;
          break;
        }
      }
    }
    ++head;
  }
  return 0;
}

// Redraws a newly spotted element and tells the other side "<name> sighted!".
void opeu_battle_report_sighted(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&               data{ Data(ds) };
  ServiceLocals const locals{ SIGHTED_RESERVE };
  auto const          message{ locals.At(MESSAGE_AT) };
  auto const          name_at{ locals.At(NAME_AT) };
  auto const&         position{ PositionOf(data, element) };
  auto const          side{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_SIDE, element)) };
  auto const          mode{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_MODE, element)) };
  Call(ds, event, BattleRoutine::BATTLE_DRAW_ELEMENT_AT, position.x, position.y, element, mode);
  auto const name{ opeu_element_name(ds, element, name_at) };
  Call(ds, event, BattleRoutine::REQUEST_FORMAT, message, Near(data, data.battle_fmt_sighted), SideColorCode(side), name);
  opeu_string_append(ds, message, Near(data, data.battle_text_sighted));
  Call(ds, event, BattleRoutine::BATTLE_COMMANDER_WAIT, opeu_opposite_side(side), 2, message);
}

// Whether the element spots the other: a chance by terrain mode, less the other cell's cover and the weather's toll.
uint16_t opeu_battle_spot_chance(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t other) {
  auto&       data{ Data(ds) };
  auto const& position{ PositionOf(data, other) };
  auto const  mode{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_TERRAIN_MODE, other, element)) };
  auto        chance{ static_cast<uint8_t>((SPOT_STEPS - mode) * SPOT_STEP) };
  auto const  terrain{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_CELL_TERRAIN, position.x, position.y)) };
  opeu_sub_byte(Cover(terrain), &chance);
  opeu_sub_byte(WeatherToll(data.weather), &chance);
  return Call(ds, event, BattleRoutine::RAND_BOUNDED, 100) < chance ? 1 : 0;
}

// The element looks around its cell: the enemies it sights are spotted and reported.
void opeu_battle_spot_enemies(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&       data{ Data(ds) };
  auto const& position{ PositionOf(data, element) };
  auto const  side{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_SIDE, element)) };
  auto const  range{ data.battle_spot_range[ElementTypeOf(data, element).category] };
  auto const  span{ static_cast<uint8_t>(range * 2 + 1) };
  auto const  left{ static_cast<int16_t>(position.x - range) };
  auto        y{ static_cast<int16_t>(position.y - range) };
  for (uint8_t row{ 0 }; row < span; ++row, ++y) {
    auto x{ left };
    for (uint8_t column{ 0 }; column < span; ++column, ++x) {
      if (x < 0 || x >= static_cast<int16_t>(BATTLE_WIDTH) || y < 0 || y >= static_cast<int16_t>(BATTLE_WIDTH)) continue;
      auto const other{ Call(ds, event, BattleRoutine::BATTLE_ELEMENT_AT, static_cast<uint16_t>(x), static_cast<uint16_t>(y)) };
      if (!other || static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_SIDE, other)) == side || Call(ds, event, BattleRoutine::BATTLE_ELEMENT_FLAG2, other)
          || !opeu_battle_spot_chance(ds, event, element, other))
        continue;
      Record<Element>(data, other).flags |= Bits(ElementFlag::SPOTTED);
      auto const own{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_SIDE, element)) };
      if (Word(ds, static_cast<uint16_t>(Near(data, data.side_status) + own * 2)) == SIDE_PLAYED) opeu_battle_report_sighted(ds, event, other);
    }
  }
}

uint16_t svc_btlpath(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_path_flood(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  auto const byte{ [&](unsigned i) { return static_cast<uint8_t>(Arg(args, count, i)); } };
  return opeu_battle_path_flood(ds, svc_btlpath, byte(0), byte(1), byte(2), byte(3), Arg(args, count, 4), byte(5));
}

auto port_routine_opeu_battle_path_step(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_path_step(ds, svc_btlpath, Arg(args, count, 1), Arg(args, count, 0));
}

auto port_routine_opeu_battle_spot_enemies(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_spot_enemies(ds, svc_btlpath, Arg(args, count, 0));
  return 0;
}
