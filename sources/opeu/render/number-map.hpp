#pragma once
// A colour lookup on the game's sixteen colour numbers: number n is shown as number map[n]. The game's XOR, AND and OR
// with a colour are such lookups (n ^ c, n & c, n | c): the renderer draws what is under them again through the
// lookup instead of computing with the pixels on the frame.

#include "opeu/assets/manifest.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>

namespace opeu::render::detail::number_map {

inline constexpr auto NUMBERS{ 16uz };

using NumberMap = std::array<std::uint8_t, NUMBERS>;

// Every number shown as itself.
inline constexpr auto SAME{ [] {
  auto map{ NumberMap{} };
  std::ranges::copy(std::views::iota(std::uint8_t{ 0 }, static_cast<std::uint8_t>(NUMBERS)), map.begin());
  return map;
}() };

// The game's operations of a colour with the number under it.
enum class Operation : std::uint8_t {
  XOR,
  AND,
  OR,
};

// n -> n op colour, for each of the sixteen numbers.
[[nodiscard]] constexpr auto Lookup(Operation const operation, std::uint8_t const colour) noexcept -> NumberMap {
  auto map{ SAME };
  std::ranges::transform(SAME, map.begin(), [operation, colour](std::uint8_t const n) {
    switch (operation) {
    case Operation::XOR: return static_cast<std::uint8_t>((n ^ colour) & 0x0F);
    case Operation::AND: return static_cast<std::uint8_t>(n & colour & 0x0F);
    case Operation::OR:  return static_cast<std::uint8_t>((n | colour) & 0x0F);
    }
    return n;
  });
  return map;
}

// Every number shown as `number`: a solid colour.
[[nodiscard]] constexpr auto Solid(std::uint8_t const number) noexcept -> NumberMap {
  auto map{ NumberMap{} };
  map.fill(static_cast<std::uint8_t>(number & 0x0F));
  return map;
}

// Whether the lookup is an XOR with some colour (such lookups undo themselves and do not depend on their order).
[[nodiscard]] constexpr auto Exchanges(NumberMap const& map) noexcept -> bool {
  return map == Lookup(Operation::XOR, map.front());
}

// `first`, then `then`: n -> then[first[n]].
[[nodiscard]] constexpr auto Then(NumberMap const& first, NumberMap const& then) noexcept -> NumberMap {
  auto map{ SAME };
  std::ranges::transform(first, map.begin(), [&then](std::uint8_t const n) { return then.at(n & 0x0F); });
  return map;
}

// Whether every number is shown as the same one (the lookup draws a solid colour), and whether none changes.
[[nodiscard]] constexpr auto Constant(NumberMap const& map) noexcept -> bool {
  return std::ranges::all_of(map, [&map](std::uint8_t const n) { return n == map.front(); });
}

// The colour each number shows through the lookup.
[[nodiscard]] constexpr auto Through(assets::PaletteColours const& colours, NumberMap const& map) noexcept -> assets::PaletteColours {
  auto shown{ colours };
  std::ranges::transform(map, shown.begin(), [&colours](std::uint8_t const n) { return colours.at(n & 0x0F); });
  return shown;
}

}

namespace opeu::render {
using detail::number_map::Constant;
using detail::number_map::Exchanges;
using detail::number_map::Lookup;
using detail::number_map::NumberMap;
using detail::number_map::NUMBERS;
using detail::number_map::Operation;
using detail::number_map::SAME;
using detail::number_map::Solid;
using detail::number_map::Then;
using detail::number_map::Through;
}
