#include "opeu/render/line-points.hpp"

#include <algorithm>
#include <ranges>

namespace opeu::render::detail::line_points {
namespace {

[[nodiscard]] constexpr auto Signed(Word const value) noexcept -> int {
  return static_cast<SignedWord>(value);
}

// The distance between two words, as the 16-bit absolute difference.
[[nodiscard]] constexpr auto Distance(Word const from, Word const to) noexcept -> Word {
  auto const difference{ static_cast<Word>(to - from) };
  return Signed(difference) < 0 ? static_cast<Word>(-difference) : difference;
}

// The walk along the major axis: where it is, its error term and the minor step.
class Walk {
public:
  Walk(LinePoint const from, LinePoint const to) {
    auto const dx{ Distance(from.x, to.x) };
    auto const dy{ Distance(from.y, to.y) };
    _x_major = dx >= dy;
    _major   = _x_major ? dx : dy;
    _minor   = _x_major ? dy : dx;
    _step    = (Signed(from.x) < Signed(to.x) && Signed(to.y) > Signed(from.y)) || (Signed(from.x) > Signed(to.x) && Signed(to.y) < Signed(from.y)) ? 1 : -1;
    _at      = (_x_major ? from.x < to.x : from.y < to.y) ? from : to;
    _error   = static_cast<Word>(_major >> 1);
  }

  [[nodiscard]] auto Length() const noexcept -> int {
    return _major + 1;
  }

  // The current point, then one step on.
  auto Next() -> LinePoint {
    auto const point{ _at };
    auto&      along{ _x_major ? _at.x : _at.y };
    auto&      across{ _x_major ? _at.y : _at.x };
    if (_error < _minor) {
      _error = static_cast<Word>(_error - _minor + _major);
      across = static_cast<Word>(across + _step);
    } else {
      _error = static_cast<Word>(_error - _minor);
    }
    ++along;
    return point;
  }

private:
  bool      _x_major{};
  Word      _major{};
  Word      _minor{};
  int       _step{};
  LinePoint _at{};
  Word      _error{};
};

}

auto LinePoints(LinePoint const from, LinePoint const to) -> std::vector<LinePoint> {
  auto walk{
    Walk{ from, to }
  };
  return std::views::iota(0, walk.Length()) | std::views::transform([&walk](int) { return walk.Next(); }) | std::ranges::to<std::vector>();
}

}
