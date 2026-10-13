// The cost of the battle calculations and the path finding on a laid-out battle field: the path flood across the
// 14 x 14 grid, walking an element down the flooded costs, the move costs and entry checks of every cell, the
// element ratings, the element at every cell, and the exit cells of the player's battle orders. Each routine is called
// directly with the service it gets in the game, the path routines on a battle stack frame as the game gives them.
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "map/btlcalc.hpp"
#include "map/btlpath.hpp"
#include "map/btlreach.hpp"
#include "services.bench/calibration.hpp"

#include "link/glue.hpp"
#include "port.hpp"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ranges>
#include <span>
#include <utility>

namespace opeu::game::bench {
namespace {

inline constexpr auto FAR_CORNER{ BATTLE_WIDTH - 1 };
// The terrain codes of open battle ground (passable for every category, at costs from 5 to 42) and an impassable one.
inline constexpr std::array<std::uint8_t, 21> GROUND{ 13, 14, 15, 16, 17, 18, 19, 20, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43 };
inline constexpr std::uint8_t                 ROCK{ 0 };
inline constexpr auto                         FLOOD_TO_CELL{ 1 }; // the flood's mode bits (map/btlpath)
inline constexpr auto                         FLOOD_THROUGH{ 2 };
inline constexpr auto                         STEPS_AT_MOST{ 2 * BATTLE_WIDTH * BATTLE_WIDTH };
inline constexpr auto                         EXIT_SHIFT{ std::to_underlying(BattleCell::EXIT_SHIFT) };
inline constexpr auto                         UNREACHED{ 0xffffu }; // a battle_path_costs entry the flood did not reach
inline constexpr auto                         ELEMENT_INDEX{ 1u };  // the element record the bench places

struct Cell {
  unsigned const x;
  unsigned const y;
};

// Every cell of the battle grid, row by row.
[[nodiscard]] auto Cells() {
  return std::views::iota(0u, BATTLE_WIDTH * BATTLE_WIDTH) | std::views::transform([](auto const at) { return Cell{ at % BATTLE_WIDTH, at / BATTLE_WIDTH }; });
}

// A battle field with one tank element standing in the top-left corner: open ground of every kind, scattered
// impassable rocks and a diagonal of occupied cells with a gap, so the flood has to go round. The data segment is the game's initial image, which holds the terrain and type tables.
class BattleField {
public:
  BattleField() : _state{ InitialSegment() }, _game{ _state, _machine }, _data{ Data(_game) } {
    std::ranges::for_each(Cells(), [this](Cell const cell) { LayCell(cell); });
    PlaceElement();
  }

  [[nodiscard]] auto Element() const -> unsigned {
    return Near(_data, _data.elements[ELEMENT_INDEX]);
  }

  // Fills the path costs from the element's cell toward the far corner.
  auto Flood(int mode) -> unsigned {
    return Flood(Cell{ 0, 0 }, Cell{ FAR_CORNER, FAR_CORNER }, mode);
  }

  // Floods from the far corner, then steps the element downhill until it stands there; returns the steps taken.
  auto WalkToFarCorner() -> unsigned {
    Flood(Cell{ FAR_CORNER, FAR_CORNER }, Cell{ 0, 0 }, FLOOD_THROUGH);
    Position() = BattlePosition{};
    auto const x_at{ Near(_data, Position()) };
    auto const steps{ std::ranges::distance(std::views::iota(0u, STEPS_AT_MOST) | std::views::take_while([&](auto) { return Step(x_at) != 0; })) };
    return static_cast<unsigned>(steps);
  }

  // The move cost of every cell plus the number it can enter.
  auto MoveCosts() -> unsigned {
    return std::ranges::fold_left(Cells() | std::views::transform([this](auto const cell) {
                                    return opeu_battle_move_cost(_game, svc_btlcalc, Byte(cell.x), Byte(cell.y), Element()) + opeu_battle_can_enter(_game, Byte(cell.x), Byte(cell.y), Element());
                                  }),
                                  0u, std::plus{});
  }

  // Strength, rating against itself and shots of the element.
  auto Ratings() -> unsigned {
    auto const element{ Element() };
    return unsigned{ opeu_battle_element_strength(_game, svc_btlcalc, element) } + opeu_battle_element_rating(_game, element, element) + opeu_battle_element_shots(_game, element, element);
  }

  // The element at every cell (the battle lists' walk).
  auto ElementsAt() -> unsigned {
    return static_cast<unsigned>(std::ranges::count_if(Cells(), [this](auto const cell) { return opeu_battle_element_at(_game, Byte(cell.x), Byte(cell.y)) != 0; }));
  }

  // The cells the last flood reached.
  [[nodiscard]] auto Reached() const -> unsigned {
    return static_cast<unsigned>(std::ranges::count_if(std::span{ _data.battle_path_costs }.first(BATTLE_WIDTH * BATTLE_WIDTH), [](auto const cost) { return cost != UNREACHED; }));
  }

  // Frames the cells the element can leave the battle from; returns whether there is one.
  auto MarkExits() -> unsigned {
    return opeu_battle_mark_exits(_game, Element());
  }

private:
  // The game's initial data segment, freshly composed in the arena.
  [[nodiscard]] static auto InitialSegment() -> SegmentBytes {
    port_memory_init();
    return SegmentBytes{ port_ds, SEGMENT_SIZE };
  }

  auto LayCell(Cell const cell) -> void {
    auto const x{ cell.x };
    auto const y{ cell.y };
    auto const rock{ (x * y) % 11 == 3 };
    BattleTerrainAt(_data, x, y) = rock ? ROCK : GROUND.at((x * 7 + y * 13 + x * y) % GROUND.size());
    auto const wall{ x + y == BATTLE_WIDTH && x != BATTLE_WIDTH / 2 };
    BattleCellAt(_data, x, y) = static_cast<std::uint8_t>((wall ? 2u : 0u) | ExitOf(cell) << EXIT_SHIFT);
  }

  // The strategic direction + 1 an element leaving from a cell of the field's edge takes (0 inside the field).
  [[nodiscard]] static auto ExitOf(Cell const cell) -> unsigned {
    if (cell.y == 0) return 1;
    if (cell.x == FAR_CORNER) return 3;
    if (cell.y == FAR_CORNER) return 5;
    if (cell.x == 0) return 7;
    return 0;
  }

  auto PlaceElement() -> void {
    auto const tanks{ std::ranges::find_if(_data.element_types, [](auto const& type) { return type.category == Bits(Category::TANKS); }) };
    auto&      element{ _data.elements[ELEMENT_INDEX] };
    element.type       = static_cast<std::uint8_t>(std::ranges::distance(_data.element_types.begin(), tanks));
    element.quantity   = 40;
    element.experience = 60;
    element.position   = Near(_data, Position());
    Position()         = BattlePosition{};
  }

  [[nodiscard]] auto Position() -> BattlePosition& {
    return Record<BattlePosition>(_data, Near(_data, _data.battle_positions));
  }

  // Fills the path costs from one cell toward another.
  auto Flood(Cell const from, Cell const to, int const mode) -> unsigned {
    return opeu_battle_path_flood(_game, Byte(from.x), Byte(from.y), Byte(to.x), Byte(to.y), Element(), Byte(mode));
  }

  // Moves the element one step down the flooded costs; x_at addresses its position's x byte, the y byte follows.
  auto Step(std::integral auto const x_at) -> unsigned {
    auto const y_at{ static_cast<decltype(x_at)>(x_at + 1) };
    return opeu_battle_path_step(_game, x_at, y_at);
  }

  [[nodiscard]] static constexpr auto Byte(std::integral auto const value) -> std::uint8_t {
    return static_cast<std::uint8_t>(value);
  }

  GameState         _state;
  services::Machine _machine;
  Game              _game;
  DataSegment&      _data;
};

auto PathFloodToCell(benchmark::State& state) -> void {
  BattleField field;
  for (auto _ : state) benchmark::DoNotOptimize(field.Flood(FLOOD_TO_CELL));
  state.counters["cells"] = field.Reached();
}
BENCHMARK(PathFloodToCell);

auto PathFloodWholeField(benchmark::State& state) -> void {
  BattleField field;
  for (auto _ : state) benchmark::DoNotOptimize(field.Flood(0));
  state.counters["cells"] = field.Reached();
}
BENCHMARK(PathFloodWholeField);

auto PathFloodThroughOccupants(benchmark::State& state) -> void {
  BattleField field;
  for (auto _ : state) benchmark::DoNotOptimize(field.Flood(FLOOD_THROUGH));
  state.counters["cells"] = field.Reached();
}
BENCHMARK(PathFloodThroughOccupants);

auto PathWalk(benchmark::State& state) -> void {
  BattleField field;
  auto        steps{ 0u };
  for (auto _ : state) steps = field.WalkToFarCorner();
  state.counters["steps"] = steps;
}
BENCHMARK(PathWalk);

auto MoveCosts(benchmark::State& state) -> void {
  BattleField field;
  for (auto _ : state) benchmark::DoNotOptimize(field.MoveCosts());
  state.SetItemsProcessed(state.iterations() * BATTLE_WIDTH * BATTLE_WIDTH);
}
BENCHMARK(MoveCosts);

auto ElementRatings(benchmark::State& state) -> void {
  BattleField field;
  for (auto _ : state) benchmark::DoNotOptimize(field.Ratings());
}
BENCHMARK(ElementRatings);

auto ElementsAt(benchmark::State& state) -> void {
  BattleField field;
  for (auto _ : state) benchmark::DoNotOptimize(field.ElementsAt());
}
BENCHMARK(ElementsAt);

auto MarkExits(benchmark::State& state) -> void {
  BattleField field;
  auto        exits{ 0u };
  for (auto _ : state) exits = field.MarkExits();
  state.counters["found"] = exits;
}
BENCHMARK(MarkExits);

} // namespace
} // namespace opeu::game::bench
