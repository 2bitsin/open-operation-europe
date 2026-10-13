// Battle path finding and enemy spotting.
//
// The path cost of a cell is its terrain's cost for the element's category (255 when occupied, unless occupants are
// ignored). The flood fills path costs from a cell over the battle grid, with a queue kept sorted by cost, until it
// reaches the other cell or runs out; the path step then moves a cell one step downhill in those costs. Spotting
// marks the enemy elements within the element's range whose chance roll (terrain mode, cover, weather) succeeds,
// and reports each to the player.
#include "map/btlpath.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/memory.hpp"
#include "core/numeric.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "map/btlcalc.hpp"
#include "map/btlcmdr.hpp"
#include "map/btldir.hpp"
#include "map/btlelem.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"
#include "units/labels.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

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

[[nodiscard]] auto TerrainCost(Game& game, uint8_t x, uint8_t y, uint16_t element) -> uint8_t {
  auto const& data{ Data(game) };
  auto const  terrain{ static_cast<uint8_t>(opeu_battle_cell_terrain(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y))) };
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

uint8_t opeu_battle_path_cost(Game& game, uint8_t x, uint8_t y, uint16_t element) {
  if (opeu_battle_cell_occupied(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y))) return BLOCKED;
  return TerrainCost(game, x, y, element);
}

uint8_t opeu_battle_path_cost_any(Game& game, uint8_t x, uint8_t y, uint16_t element) {
  return TerrainCost(game, x, y, element);
}

// x, y: DS offsets of the cell bytes, moved to the cheapest neighbour. With no reachable neighbour the cell takes the
// reserve's stale bytes, as the original's uninitialized locals.
uint16_t opeu_battle_path_step(Game& game, uint16_t x, uint16_t y) {
  auto const&         data{ Data(game) };
  ServiceLocals const locals{ STEP_RESERVE };
  auto const          step_x_at{ locals.At(STEP_X_AT) };
  auto const          step_y_at{ locals.At(STEP_Y_AT) };
  uint16_t            best{ UNREACHED };
  uint8_t             best_x{ 0 };
  uint8_t             best_y{ 0 };
  for (uint8_t direction{ 0 }; direction < DIRECTIONS; ++direction) {
    Byte(game, step_x_at) = Byte(game, x);
    Byte(game, step_y_at) = Byte(game, y);
    if (!opeu_battle_step(game, game.State().Bytes(step_x_at, 1).data(), game.State().Bytes(step_y_at, 1).data(), static_cast<uint8_t>(direction))) continue;
    auto const cost{ data.battle_path_costs[Byte(game, step_y_at) * BATTLE_WIDTH + Byte(game, step_x_at)] };
    if (cost < best) {
      best_x = Byte(game, step_x_at);
      best_y = Byte(game, step_y_at);
      best   = cost;
    }
  }
  if (best == UNREACHED) {
    best_x = Byte(game, locals.At(STEP_STALE_X_AT));
    best_y = Byte(game, locals.At(STEP_STALE_Y_AT));
  }
  Byte(game, x) = best_x;
  Byte(game, y) = best_y;
  return best;
}

// Fills battle_path_costs from (x, y). Returns 1 when the flood reached (to_x, to_y) with FLOOD_TO_CELL set.
uint16_t opeu_battle_path_flood(Game& game, uint8_t x, uint8_t y, uint8_t to_x, uint8_t to_y, uint16_t element, uint8_t mode) {
  auto&               data{ Data(game) };
  auto&               queue{ data.battle_path_queue };
  auto&               costs{ data.battle_path_costs };
  ServiceLocals const locals{ FLOOD_RESERVE };
  auto const          step_x_at{ locals.At(FLOOD_X_AT) };
  auto const          step_y_at{ locals.At(FLOOD_Y_AT) };
  unsigned            head{ 0 };
  unsigned            tail{ 1 };
  queue[head] = static_cast<uint8_t>(x + y * BATTLE_WIDTH);
  opeu_fill_bytes(game, Near(data, costs), CELLS * 2, BLOCKED);
  costs[queue[0]] = 0;
  while (tail != head) {
    auto const from_x{ static_cast<uint8_t>(queue[head] % BATTLE_WIDTH) };
    auto const from_y{ static_cast<uint8_t>(queue[head] / BATTLE_WIDTH) };
    auto const cost{ costs[from_y * BATTLE_WIDTH + from_x] };
    for (uint8_t direction{ 0 }; direction < DIRECTIONS; ++direction) {
      Byte(game, step_x_at) = from_x;
      Byte(game, step_y_at) = from_y;
      if (!opeu_battle_step(game, game.State().Bytes(step_x_at, 1).data(), game.State().Bytes(step_y_at, 1).data(), static_cast<uint8_t>(direction))) continue;
      auto const step_x{ Byte(game, step_x_at) };
      auto const step_y{ Byte(game, step_y_at) };
      if (to_x == step_x && to_y == step_y && (mode & FLOOD_TO_CELL)) return 1;
      auto const cell{ static_cast<uint8_t>(step_y * BATTLE_WIDTH + step_x) };
      if (costs[cell] != UNREACHED) continue;
      auto const entry{ mode & FLOOD_THROUGH ? opeu_battle_path_cost_any(game, step_x, step_y, element) : opeu_battle_path_cost(game, step_x, step_y, element) };
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
void opeu_battle_report_sighted(Game& game, uint16_t element) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ SIGHTED_RESERVE };
  auto const          message{ locals.At(MESSAGE_AT) };
  auto const          name_at{ locals.At(NAME_AT) };
  auto const&         position{ PositionOf(data, element) };
  auto const          side{ static_cast<uint8_t>(opeu_battle_element_side(game, element)) };
  auto const          mode{ static_cast<uint8_t>(opeu_battle_mode(game, element)) };
  opeu_battle_draw_element_at(game, static_cast<uint8_t>(position.x), static_cast<uint8_t>(position.y), element, static_cast<uint8_t>(mode));
  auto const name{ opeu_element_name(game, element, name_at) };
  opeu_request_format(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(message), static_cast<std::uint16_t>(Near(data, data.battle_fmt_sighted)),
                                                          static_cast<std::uint16_t>(SideColorCode(side)), static_cast<std::uint16_t>(name) });
  opeu_string_append(game, message, Near(data, data.battle_text_sighted));
  opeu_battle_commander_wait(game, message, opeu_opposite_side(side), 2);
}

// Whether the element spots the other: a chance by terrain mode, less the other cell's cover and the weather's toll.
uint16_t opeu_battle_spot_chance(Game& game, uint16_t element, uint16_t other) {
  auto&       data{ Data(game) };
  auto const& position{ PositionOf(data, other) };
  auto const  mode{ static_cast<uint8_t>(opeu_battle_terrain_mode(game, element, other)) };
  auto        chance{ static_cast<uint8_t>((SPOT_STEPS - mode) * SPOT_STEP) };
  auto const  terrain{ static_cast<uint8_t>(opeu_battle_cell_terrain(game, static_cast<uint8_t>(position.x), static_cast<uint8_t>(position.y))) };
  opeu_sub_byte(Cover(terrain), &chance);
  opeu_sub_byte(WeatherToll(data.weather), &chance);
  return port_rand_bounded(game, 100) < chance ? 1 : 0;
}

// The element looks around its cell: the enemies it sights are spotted and reported.
void opeu_battle_spot_enemies(Game& game, uint16_t element) {
  auto&       data{ Data(game) };
  auto const& position{ PositionOf(data, element) };
  auto const  side{ static_cast<uint8_t>(opeu_battle_element_side(game, element)) };
  auto const  range{ data.battle_spot_range[ElementTypeOf(data, element).category] };
  auto const  span{ static_cast<uint8_t>(range * 2 + 1) };
  auto const  left{ static_cast<int16_t>(position.x - range) };
  auto        y{ static_cast<int16_t>(position.y - range) };
  for (uint8_t row{ 0 }; row < span; ++row, ++y) {
    auto x{ left };
    for (uint8_t column{ 0 }; column < span; ++column, ++x) {
      if (x < 0 || x >= static_cast<int16_t>(BATTLE_WIDTH) || y < 0 || y >= static_cast<int16_t>(BATTLE_WIDTH)) continue;
      auto const other{ opeu_battle_element_at(game, static_cast<uint8_t>(static_cast<uint16_t>(x)), static_cast<uint8_t>(static_cast<uint16_t>(y))) };
      if (!other || static_cast<uint8_t>(opeu_battle_element_side(game, other)) == side || opeu_battle_element_flag2(game, other) || !opeu_battle_spot_chance(game, element, other)) continue;
      Record<Element>(data, other).flags |= Bits(ElementFlag::SPOTTED);
      auto const own{ static_cast<uint8_t>(opeu_battle_element_side(game, element)) };
      if (Word(game, static_cast<uint16_t>(Near(data, data.side_status) + own * 2)) == SIDE_PLAYED) opeu_battle_report_sighted(game, other);
    }
  }
}
