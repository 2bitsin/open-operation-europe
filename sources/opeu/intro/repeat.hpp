// Loops whose end is a condition, not a count, written as an algorithm over an unbounded count (ranges and algorithms
// before hand-written loops): the steps run in order, one at a time, and the first that says so ends the loop.
#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <ranges>
#include <tuple>

namespace opeu::intro::detail::repeat {

// Runs `step` until it returns true: a loop whose test follows its body.
template <class _Step>
auto RepeatUntil(_Step&& step) -> void {
  std::ignore = std::ranges::find_if(std::views::iota(std::uint64_t{ 0 }), [&](std::uint64_t) { return std::invoke(step); });
}

// Runs `body` while `condition` holds: a loop whose test precedes its body.
template <class _Condition, class _Body>
auto RepeatWhile(_Condition&& condition, _Body&& body) -> void {
  RepeatUntil([&] {
    if (!std::invoke(condition)) return true;
    std::invoke(body);
    return false;
  });
}

} // namespace opeu::intro::detail::repeat

namespace opeu::intro {
using detail::repeat::RepeatUntil;
using detail::repeat::RepeatWhile;
} // namespace opeu::intro
