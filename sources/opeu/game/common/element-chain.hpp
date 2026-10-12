// The elements of a unit as a range: a unit's first and each element's next are near pointers, 0 at the end.
#pragma once

#include "common/data-segment.hpp"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace opeu::game::detail::element_chain {

// Walks Element records from a near pointer; Segment is DataSegment or DataSegment const. The next link is read when
// the iterator advances, so the loop body may relink the element it holds.
template <typename Segment> class ElementChain {
public:
  using Value = std::conditional_t<std::is_const_v<Segment>, Element const, Element>;

  class Iterator {
  public:
    using value_type      = Element;
    using difference_type = std::ptrdiff_t;

    Iterator() = default;
    Iterator(Segment* data, std::uint16_t near) noexcept : _data{ data }, _near{ near } { }

    [[nodiscard]] auto operator*() const noexcept -> Value& {
      return Record<Element>(*_data, _near);
    }
    auto operator++() noexcept -> Iterator& {
      _near = (**this).next;
      return *this;
    }
    auto operator++(int) noexcept -> Iterator {
      auto const before{ *this };
      ++*this;
      return before;
    }
    [[nodiscard]] auto operator==(std::default_sentinel_t) const noexcept -> bool {
      return _near == 0;
    }
    // The near pointer of the element the iterator holds.
    [[nodiscard]] auto Near() const noexcept -> std::uint16_t {
      return _near;
    }

  private:
    Segment*      _data{};
    std::uint16_t _near{};
  };

  ElementChain(Segment& data, std::uint16_t first) noexcept : _data{ &data }, _first{ first } { }

  [[nodiscard]] auto begin() const noexcept -> Iterator {
    return { _data, _first };
  }
  [[nodiscard]] auto end() const noexcept -> std::default_sentinel_t {
    return {};
  }

private:
  Segment*      _data;
  std::uint16_t _first;
};

// The chain starting at the element `first` points to (a Unit's first or an Element's next).
[[nodiscard]] inline auto Elements(DataSegment& data, std::uint16_t first) noexcept -> ElementChain<DataSegment> {
  return { data, first };
}
[[nodiscard]] inline auto Elements(DataSegment const& data, std::uint16_t first) noexcept
    -> ElementChain<DataSegment const> {
  return { data, first };
}
// The elements of a unit.
[[nodiscard]] inline auto Elements(DataSegment& data, Unit const& unit) noexcept -> ElementChain<DataSegment> {
  return { data, unit.first };
}
[[nodiscard]] inline auto Elements(DataSegment const& data, Unit const& unit) noexcept
    -> ElementChain<DataSegment const> {
  return { data, unit.first };
}
} // namespace opeu::game::detail::element_chain

namespace opeu::game {
using detail::element_chain::ElementChain;
using detail::element_chain::Elements;
} // namespace opeu::game
