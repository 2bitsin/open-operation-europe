#pragma once
// Pixels row by row: a surface's or an image's bytes as `count` rows of `width` bytes, `pitch` bytes apart.

#include <cstddef>
#include <ranges>
#include <span>

namespace opeu::render::detail::rows {

struct RowLayout {
  std::size_t pitch{}; // bytes from one row's start to the next
  std::size_t width{}; // bytes of a row that are pixels
  std::size_t count{}; // rows
};

template <class _Byte>
[[nodiscard]] constexpr auto Rows(std::span<_Byte> const bytes, RowLayout const layout) {
  return std::views::iota(0uz, layout.count) | std::views::transform([bytes, layout](std::size_t const n) { return bytes.subspan(n * layout.pitch, layout.width); });
}

}

namespace opeu::render {
using detail::rows::RowLayout;
using detail::rows::Rows;
}
