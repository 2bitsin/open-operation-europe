#include "opeu/game/common/game.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace opeu::game::detail::game_test {
namespace {

// A zeroed data segment, the state over it and a game on an empty machine: no arena, no globals.
class Fixture {
public:
  Fixture() : _bytes{ std::make_unique<std::array<std::uint8_t, SEGMENT_SIZE>>() }, _state{ SegmentBytes{ *_bytes } }, _game{ _state, _machine } { }

  [[nodiscard]] auto Bytes() -> std::array<std::uint8_t, SEGMENT_SIZE>& {
    return *_bytes;
  }
  [[nodiscard]] auto Game() -> game::Game& {
    return _game;
  }

private:
  std::unique_ptr<std::array<std::uint8_t, SEGMENT_SIZE>> _bytes;
  GameState                                               _state;
  services::Machine                                       _machine;
  game::Game                                              _game;
};
} // namespace

TEST(Game, ViewsTheSegmentBytesAsTypedRecords) {
  Fixture fixture;
  fixture.Bytes()[offsetof(DataSegment, scenario)] = 0x80;
  EXPECT_EQ(Data(fixture.Game()).scenario, 0x80);
  Data(fixture.Game()).month = 5;
  EXPECT_EQ(fixture.Bytes()[offsetof(DataSegment, month)], 5);
}

TEST(Game, ReadsAndWritesLittleEndianWords) {
  Fixture fixture;
  EXPECT_EQ(SetWord(fixture.Game(), 0x1234, 0xbeef), 0xbeef);
  EXPECT_EQ(fixture.Bytes()[0x1234], 0xef);
  EXPECT_EQ(fixture.Bytes()[0x1235], 0xbe);
  EXPECT_EQ(Word(fixture.Game(), 0x1234), 0xbeef);
}

TEST(Game, WrapsAWordsHighByteAtTheEndOfTheSegment) {
  Fixture fixture;
  SetWord(fixture.Game(), 0xffff, 0x1234);
  EXPECT_EQ(fixture.Bytes()[0xffff], 0x34);
  EXPECT_EQ(fixture.Bytes()[0x0000], 0x12);
  EXPECT_EQ(Word(fixture.Game(), 0xffff), 0x1234);
}

TEST(Game, HandsOutAByteByItsNearAddress) {
  Fixture fixture;
  Byte(fixture.Game(), 0x0100) = 7;
  EXPECT_EQ(fixture.Bytes()[0x0100], 7);
}

TEST(GameState, ViewsANearBufferInPlace) {
  Fixture    fixture;
  auto const buffer{ fixture.Game().State().Bytes(0x2000, 4) };
  std::ranges::fill(buffer, std::uint8_t{ 9 });
  EXPECT_EQ(buffer.size(), 4u);
  EXPECT_TRUE(std::ranges::all_of(std::span{ fixture.Bytes() }.subspan(0x2000, 4), [](auto const byte) { return byte == 9; }));
  EXPECT_EQ(fixture.Bytes()[0x2004], 0);
}

TEST(GameState, ViewsTheRestOfTheSegmentFromANearAddress) {
  Fixture fixture;
  EXPECT_EQ(fixture.Game().State().Bytes(0xfff0).size(), 0x10u);
  EXPECT_EQ(fixture.Game().State().Bytes(0xfff0).data(), &fixture.Bytes()[0xfff0]);
}
} // namespace opeu::game::detail::game_test
