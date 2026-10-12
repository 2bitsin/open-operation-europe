// The start-up data image of the data segment: each data module writes its fields' initial values into the zeroed
// arena (link/image.cpp), the far data modules' objects are copied to their segments (link/gen/data-image.cpp).
#pragma once

#include "common/data-segment.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace opeu::game::detail::image {

// The near pointer of element `index`.
constexpr auto ElementNear(std::uint16_t index) -> std::uint16_t {
  return static_cast<std::uint16_t>(ELEMENTS_AT + index * sizeof(Element));
}

// Copies the text into the start of the field; a text as long as the field has no terminator.
template <std::size_t N, std::size_t M>
constexpr auto SetText(std::array<char, N>& field, char const (&text)[M]) -> void {
  static_assert(M - 1 <= N, "the text is longer than its field");
  std::copy_n(text, M - 1, field.begin());
}

// Passes each value with its index to `set`, which stores it (the packed segment's word arrays take no reference).
template <typename T, std::size_t N, typename Set>
constexpr auto SetEach(std::array<T, N> const& values, Set set) -> void {
  [&]<std::size_t... INDEX>(std::index_sequence<INDEX...>) { (set(INDEX, values[INDEX]), ...); }(std::make_index_sequence<N>{});
}

// Writes the start-up values of every data module that lives in the data segment.
auto InitialDataSegment(DataSegment& data) -> void;
} // namespace opeu::game::detail::image

namespace opeu::game {
using detail::image::ElementNear;
using detail::image::InitialDataSegment;
using detail::image::SetEach;
using detail::image::SetText;
} // namespace opeu::game
