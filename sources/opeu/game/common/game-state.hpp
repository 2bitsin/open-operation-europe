// The simulation's state: what a saved game or a determinism check compares. Today it is MAIN's 64 KiB data segment
// as typed records (common/data-segment.hpp), viewed where the arena holds it (#14a); it owns the segment once nothing
// addresses the data segment as far memory any more (#14b), and M4 splits it into the game's domain objects.
#pragma once

#include "common/data-segment.hpp"

#include "opeu/contract.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace opeu::game::detail::game_state {

inline constexpr std::size_t SEGMENT_SIZE{ 0x10000 }; // the data segment's bytes: every 16-bit near address

using SegmentBytes = std::span<std::uint8_t, SEGMENT_SIZE>;

class GameState {
public:
  explicit GameState(SegmentBytes segment) noexcept : _segment{ segment } { }

  [[nodiscard]] auto Data() noexcept -> DataSegment& {
    return data_segment::Data(_segment.data());
  }
  [[nodiscard]] auto Data() const noexcept -> DataSegment const& {
    return data_segment::Data(static_cast<std::uint8_t const*>(_segment.data()));
  }

  // The byte at a near address: a byte variable a routine receives by its address.
  [[nodiscard]] auto Byte(std::uint16_t near) noexcept -> std::uint8_t& {
    return _segment[near];
  }
  [[nodiscard]] auto Byte(std::uint16_t near) const noexcept -> std::uint8_t const& {
    return _segment[near];
  }

  // The `size` bytes at a near address: a buffer or a string the game hands to a service by its near address.
  [[nodiscard]] auto Bytes(std::uint16_t near, std::size_t size) -> std::span<std::uint8_t> {
    Expects(near + size <= SEGMENT_SIZE, "a near buffer lies inside the data segment");
    return std::span{ _segment }.subspan(near, size);
  }
  [[nodiscard]] auto Bytes(std::uint16_t near, std::size_t size) const -> std::span<std::uint8_t const> {
    Expects(near + size <= SEGMENT_SIZE, "a near buffer lies inside the data segment");
    return std::span<std::uint8_t const>{ _segment }.subspan(near, size);
  }
  // The bytes from a near address to the end of the segment: a near pointer whose extent only its callee knows (a
  // glyph, a palette, a string), until the callee takes a typed view.
  [[nodiscard]] auto Bytes(std::uint16_t near) noexcept -> std::span<std::uint8_t> {
    return std::span{ _segment }.subspan(near);
  }

private:
  SegmentBytes _segment;
};
} // namespace opeu::game::detail::game_state

namespace opeu::game {
using detail::game_state::GameState;
using detail::game_state::SEGMENT_SIZE;
using detail::game_state::SegmentBytes;
} // namespace opeu::game
