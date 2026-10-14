// The BIOS keyboard buffer, the mouse driver and the console read behave as the game's DOS devices did.
#include "input-devices.hpp"
#include "console-read.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

namespace opeu::services {
namespace {

TEST(KeyBuffer, HandsOutKeysInTheOrderPressed) {
  KeyBuffer keys;
  EXPECT_FALSE(keys.Waiting());
  keys.Push(0x1E, 'a');
  keys.Push(0x48, 0);
  ASSERT_TRUE(keys.Waiting());
  EXPECT_EQ(keys.Take(), 0x1E61);
  EXPECT_EQ(keys.Take(), 0x4800);
  EXPECT_FALSE(keys.Waiting());
}

TEST(KeyBuffer, DropsTheSixteenthKeyLikeTheBios) {
  KeyBuffer keys;
  std::ranges::for_each(std::views::iota(0, 16), [&keys](int const n) { keys.Push(static_cast<std::uint8_t>(n + 1), 0); });
  auto const taken{ std::views::iota(0, 15) | std::views::transform([&keys](int) { return keys.Take(); }) | std::ranges::to<std::vector>() };
  EXPECT_EQ(taken.front(), 0x0100);
  EXPECT_EQ(taken.back(), 0x0F00);
  EXPECT_FALSE(keys.Waiting());
}

TEST(MouseDriver, ResetsHiddenAtTheCentre) {
  MouseDriver mouse;
  mouse.Move({ .x = 10, .y = 20 });
  mouse.Reset();
  EXPECT_EQ(mouse.State(), (MouseState{ .buttons = 0, .x = 320, .y = 240 }));
  std::vector<std::uint8_t>                                  frame(detail::hostinput::FRAME_PIXELS);
  std::array<std::uint8_t, detail::hostinput::PALETTE_BYTES> palette{};
  EXPECT_FALSE(mouse.Draw(std::span<std::uint8_t, detail::hostinput::FRAME_PIXELS>{ frame.data(), frame.size() }, palette));
}

TEST(MouseDriver, ReportsAPressReleasedBeforeTheQueryOnce) {
  MouseDriver mouse;
  mouse.Button({ .button = 0, .down = true, .x = 5, .y = 6 });
  mouse.Button({ .button = 0, .down = false, .x = 5, .y = 6 });
  EXPECT_EQ(mouse.State(), (MouseState{ .buttons = 1, .x = 5, .y = 6 }));
  EXPECT_EQ(mouse.State().buttons, 0);
}

TEST(MouseDriver, ClampsToRangesGivenInEitherOrder) {
  MouseDriver mouse;
  mouse.SetXRange(MouseRange::Between({ .min = 300, .max = 100 }));
  mouse.SetYRange(MouseRange::Between({ .min = 50, .max = 60 }));
  mouse.Move({ .x = 639, .y = 0 });
  EXPECT_EQ(mouse.State(), (MouseState{ .buttons = 0, .x = 300, .y = 50 }));
}

TEST(MouseDriver, DrawsItsArrowOnceShownAndOnlyWhenMoved) {
  MouseDriver mouse;
  mouse.Show();
  mouse.Move({ .x = 100, .y = 100 });
  ASSERT_TRUE(mouse.Dirty());
  std::vector<std::uint8_t>                                  frame(detail::hostinput::FRAME_PIXELS);
  std::array<std::uint8_t, detail::hostinput::PALETTE_BYTES> palette{};
  EXPECT_TRUE(mouse.Draw(std::span<std::uint8_t, detail::hostinput::FRAME_PIXELS>{ frame.data(), frame.size() }, palette));
  EXPECT_EQ(frame.at((100uz * 640) + 100), 254); // the tip's outline
  EXPECT_EQ(palette.at(255uz * 3), 255);         // the body is white
  EXPECT_FALSE(mouse.Dirty());
  mouse.Move({ .x = 101, .y = 100 });
  EXPECT_TRUE(mouse.Dirty());
}

TEST(ConsoleRead, ReturnsAnExtendedKeyAsZeroThenItsScanCode) {
  ConsoleRead                       console;
  std::vector<std::uint16_t>        keys{ 0x4800, 0x1E61 };
  auto                              next{ keys.begin() };
  auto const                        read{ [&next] { return *next++; } };
  std::array<std::uint8_t, 3> const read_back{ console.Next(read), console.Next(read), console.Next(read) };
  EXPECT_EQ(read_back, (std::array<std::uint8_t, 3>{ 0, 0x48, 'a' }));
}

}
}
