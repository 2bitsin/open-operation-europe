// The request-cell scan: clears the candidate grid, then for each map border the side may enter from (its eight
// request_directions) runs rays in from that border, each ray stopping where the predicate says so, and counts the
// candidates the predicate marked. Rays that run toward coordinate 0 stop before it.
#include "map/reqcells.hpp"
#include "common/data-segment.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

#include <algorithm>
#include <ranges>

using namespace opeu::game;

namespace {
constexpr uint8_t HALF{ MAP_WIDTH / 2 };

// Where a ray starts: at coordinate 0 running up, or at 39 running down (it stops before 0).
enum class Ray {
  UP,
  DOWN,
};

// The rays of one scan: each visits cells along a row or a column until the predicate stops it.
class RayScan {
public:
  RayScan(Game& game, uint8_t side, CellPredicate& visit) noexcept : _game{ game }, _side{ side }, _visit{ visit } { }

  // Rays along x for the rows [begin, end).
  void Rows(uint8_t begin, uint8_t end, Ray ray) const {
    for (auto const y : std::views::iota(begin, end)) Run(ray, [&](uint8_t x) { return _visit(_game, x, y, _side) != 0; });
  }

  // Rays along y for the columns [begin, end).
  void Columns(uint8_t begin, uint8_t end, Ray ray) const {
    for (auto const x : std::views::iota(begin, end)) Run(ray, [&](uint8_t y) { return _visit(_game, x, y, _side) != 0; });
  }

private:
  // Visits the ray's cells until one stops it.
  template <typename _Stops>
  static void Run(Ray ray, _Stops const& stops) {
    if (ray == Ray::UP)
      VisitUntil(std::views::iota(uint8_t{ 0 }, uint8_t{ MAP_WIDTH }), stops);
    else
      VisitUntil(std::views::iota(uint8_t{ 1 }, uint8_t{ MAP_WIDTH }) | std::views::reverse, stops);
  }

  template <typename _Cells, typename _Stops>
  static void VisitUntil(_Cells cells, _Stops const& stops) {
    for (auto const cell : cells)
      if (stops(cell)) return;
  }

  Game&          _game;
  uint8_t        _side;
  CellPredicate& _visit;
};
} // namespace

uint16_t opeu_count_request_cells(Game& game) {
  return static_cast<uint16_t>(std::ranges::count(Data(game).arrival_candidates, uint8_t{ 1 }));
}

uint16_t opeu_request_cells(Game& game, uint8_t side, CellPredicate& predicate) {
  auto& data{ Data(game) };
  data.arrival_candidates.fill(0);
  auto const    enabled{ [&](unsigned border) { return data.request_directions[side * 8 + border] == 1; } };
  RayScan const scan{ game, side, predicate };
  if (enabled(0)) {
    scan.Rows(HALF, MAP_HEIGHT, Ray::UP);
    scan.Columns(0, HALF, Ray::DOWN);
  }
  if (enabled(1)) scan.Columns(0, MAP_WIDTH, Ray::DOWN);
  if (enabled(2)) {
    scan.Columns(HALF, MAP_WIDTH, Ray::DOWN);
    scan.Rows(HALF, MAP_HEIGHT, Ray::DOWN);
  }
  if (enabled(3)) scan.Rows(0, MAP_HEIGHT, Ray::UP);
  if (enabled(4)) scan.Rows(0, MAP_HEIGHT, Ray::DOWN);
  if (enabled(5)) {
    scan.Rows(0, HALF, Ray::UP);
    scan.Columns(0, HALF, Ray::UP);
  }
  if (enabled(6)) scan.Columns(0, MAP_WIDTH, Ray::UP);
  if (enabled(7)) {
    scan.Columns(HALF, MAP_WIDTH, Ray::UP);
    scan.Rows(0, HALF, Ray::DOWN);
  }
  return opeu_count_request_cells(game);
}
