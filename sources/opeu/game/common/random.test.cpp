#include "opeu/game/common/random.hpp"
#include "opeu/game/common/game-state.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace opeu::game::detail::random_test {
namespace {

// A zeroed data segment and the state over it.
class Fixture {
public:
  Fixture() : _bytes{ std::make_unique<std::array<std::uint8_t, SEGMENT_SIZE>>() }, _state{ SegmentBytes{ *_bytes } } { }

  [[nodiscard]] auto State() -> GameState& {
    return _state;
  }
  [[nodiscard]] auto Stored() const -> RandomState {
    auto const at{ offsetof(DataSegment, random_state) };
    return { (*_bytes)[at], (*_bytes)[at + 1], (*_bytes)[at + 2], (*_bytes)[at + 3] };
  }

private:
  std::unique_ptr<std::array<std::uint8_t, SEGMENT_SIZE>> _bytes;
  GameState                                               _state;
};
} // namespace

TEST(RandomGenerator, DrawsTheCRuntimeSequence) {
  RandomGenerator generator;
  generator.Seed(1);
  EXPECT_EQ(generator.Next(), 16838);
  EXPECT_EQ(generator.Next(), 5758);
  EXPECT_EQ(generator.Next(), 10113);
}

TEST(RandomGenerator, AZeroBoundGivesZeroWithoutDrawing) {
  RandomGenerator generator;
  generator.Seed(1);
  EXPECT_EQ(generator.Bounded(0), 0);
  EXPECT_EQ(generator.Bounded(100), 16838 % 100);
}

TEST(RandomGenerator, StoresAndLoadsItsState) {
  RandomGenerator first;
  first.Seed(0x1234);
  RandomGenerator second;
  second.Load(first.Stored());
  EXPECT_EQ(first.Next(), second.Next());
}

TEST(GameState, ASynchronisedDrawContinuesTheDataSegmentsSequence) {
  Fixture fixture;
  fixture.State().RandomSeed(1);
  fixture.State().Generator().Seed(99);
  EXPECT_EQ(fixture.State().RandomBounded(0x8000), 16838);
  EXPECT_EQ(fixture.State().RandomNext(), 5758);
}

TEST(GameState, AGeneratorDrawLeavesTheDataSegmentsCopy) {
  Fixture fixture;
  fixture.State().RandomSeed(1);
  auto const stored{ fixture.Stored() };
  fixture.State().Generator().Next();
  EXPECT_EQ(fixture.Stored(), stored);
  fixture.State().RandomNext();
  EXPECT_NE(fixture.Stored(), stored);
}

} // namespace opeu::game::detail::random_test
