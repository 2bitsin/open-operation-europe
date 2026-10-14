// The simulation's state: what a saved game or a determinism check compares. Today it is MAIN's 64 KiB data segment
// as typed records (common/data-segment.hpp), owned on the heap, and the random generator; M4 splits it into the game's
// domain objects.
#pragma once

#include "common/data-segment.hpp"
#include "common/random.hpp"

#include "opeu/contract.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace opeu::game::detail::game_state {

inline constexpr std::size_t SEGMENT_SIZE{ 0x10000 }; // the data segment's bytes: every 16-bit near address

using SegmentBytes = std::span<std::uint8_t, SEGMENT_SIZE>;

class GameState {
public:
  // A zeroed data segment: the state a test fills in field by field.
  GameState() : _data{ std::make_unique<DataSegment>() }, _segment{ BytesOf(*_data) } { }
  // The state the game starts from: the data modules' fields and the data segment's share of the data image
  // (link/image.cpp).
  [[nodiscard]] static auto Initial() -> GameState;

  [[nodiscard]] auto Data() noexcept -> DataSegment& {
    return *_data;
  }
  [[nodiscard]] auto Data() const noexcept -> DataSegment const& {
    return *_data;
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

  // The generator itself: its draws leave the data segment's copy of the state as it was (the battle modules draw
  // this way, so a battle's numbers do not reach the saved game until the next synchronised draw).
  [[nodiscard]] auto Generator() noexcept -> RandomGenerator& {
    return _generator;
  }

  // A number below `bound`, drawn from the state the data segment keeps: the generator takes that state, draws, and
  // the data segment takes the generator's state back.
  auto RandomBounded(std::uint16_t bound) -> std::uint16_t {
    return Synchronised([bound](RandomGenerator& generator) { return generator.Bounded(bound); });
  }
  // The next number of the data segment's sequence, as RandomBounded draws it.
  auto RandomNext() -> std::uint16_t {
    return Synchronised([](RandomGenerator& generator) { return generator.Next(); });
  }
  // Seeds the generator and stores its state in the data segment.
  auto RandomSeed(std::uint16_t seed) -> void {
    _generator.Seed(seed);
    StoreRandomState();
  }

private:
  // The segment's bytes, which the near addresses index.
  static auto BytesOf(DataSegment& data) noexcept -> SegmentBytes {
    return SegmentBytes{ reinterpret_cast<std::uint8_t*>(&data), SEGMENT_SIZE }; // NOLINT(*-reinterpret-cast): the records' bytes
  }

  auto Synchronised(auto draw) -> std::uint16_t {
    RandomState stored;
    std::ranges::copy(std::span{ Data().random_state }.first<stored.size()>(), stored.begin());
    _generator.Load(stored);
    auto const result{ draw(_generator) };
    StoreRandomState();
    return result;
  }
  auto StoreRandomState() -> void {
    std::ranges::copy(_generator.Stored(), Data().random_state.begin());
  }

  std::unique_ptr<DataSegment> _data;
  SegmentBytes                 _segment;
  RandomGenerator              _generator;
};
} // namespace opeu::game::detail::game_state

namespace opeu::game {
using detail::game_state::GameState;
using detail::game_state::SEGMENT_SIZE;
using detail::game_state::SegmentBytes;
} // namespace opeu::game
