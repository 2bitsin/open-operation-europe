// The frame's recipe stays bounded however long the game plays: a step that covers earlier ones replaces them, a
// hidden overlay and a paired XOR leave nothing behind, and every frame's painting is flushed, so a long run through
// the headless renderer neither grows its recipe nor queues SDL's drawing without end (docs/rendering.md, "Flush
// discipline").
#include "display-differential.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

namespace opeu::render::detail::bounded_recipe_test {

constexpr auto FRAMES{ 2000uz };
constexpr auto BOUND{ 64uz };

using services::Area;
using services::Bar;
using services::Overlay;
using services::Point;

// One tick of what the game does over and over: a panel cleared and written on, a menu bar shown and hidden, a cursor
// XORed on and off, a dialog's ground saved and put back, a battle cell taken and put back, the frame shown.
auto Tick(RenderDisplay& display, std::size_t const frame) -> void {
  auto const row{ static_cast<Word>(60 + frame % 8 * 16) };
  std::ignore = display.Box({ .x = 10, .y = 60, .width = 50, .height = 160 }, static_cast<Word>(1 + frame % 7));
  std::ignore = display.Box({ .x = 12, .y = row, .width = 20, .height = 8 }, 4);
  auto const bar{ display.ShowOverlay(Overlay{ .shape = Bar{ .columns = Area{ .x = 11, .y = row, .width = 30, .height = 16 } }, .exchange = 0x0F }) };
  std::ignore = display.Highlight({ .x = 13, .y = 88, .width = 3, .height = 24 }, 0x010F);
  std::ignore = display.Highlight({ .x = 13, .y = 88, .width = 3, .height = 24 }, 0x010F);
  auto const ground{ display.Save(Area{ .x = 20, .y = 240, .width = 30, .height = 100 }) };
  std::ignore = display.Box({ .x = 20, .y = 240, .width = 30, .height = 100 }, 9);
  display.Restore(ground, Point{ .x = 20, .y = 240 });
  display.PutCell(Point{ .x = 40, .y = 300 }, display.TakeCell(Point{ .x = 40, .y = 300 }));
  display.HideOverlay(bar);
  display.Present();
}

TEST(BoundedRecipe, ALongRunKeepsTheRecipeAsLongAsOneTick) {
  auto  rendered{ test::Rendered{} };
  auto& display{ rendered.Screen() };
  display.EnterGraphics();
  display.ShowIdentityPalette();
  auto frame{ std::vector<std::uint8_t>(FRAME_PIXELS) };
  auto palette{ std::array<std::uint8_t, PALETTE_LEVELS>{} };
  auto sizes{ std::vector<std::size_t>{} };
  std::ranges::for_each(std::views::iota(0uz, FRAMES), [&](std::size_t const n) {
    Tick(display, n);
    std::ignore = display.RenderIndexed(std::span<std::uint8_t, FRAME_PIXELS>{ frame.data(), FRAME_PIXELS }, palette);
    sizes.push_back(display.RecipeSize());
  });
  EXPECT_LE(std::ranges::max(sizes), BOUND);
  EXPECT_LE(sizes.back(), sizes.at(FRAMES / 10));
}

}
