#include "demo-scene.hpp"

#include "opeu/render/atlas-text-renderer.hpp"
#include "opeu/render/baking.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::probes::render_demo::detail::demo_scene {
namespace {

using render::Point;
using render::Scene;
using render::Sprite;

constexpr auto HOURS{
  std::array{ std::string_view{ "main/dawn" }, std::string_view{ "main/day" }, std::string_view{ "main/dusk" }, std::string_view{ "main/night" } }
};
constexpr auto TILES_PER_ROW{ 24 };
constexpr auto TILE{ 24 };

auto AddTiles(Scene& scene) -> void {
  for (auto const [row, hour] : std::views::zip(std::views::iota(0), HOURS)) {
    std::ranges::for_each(std::views::iota(0, TILES_PER_ROW), [&scene, row, hour](int const n) {
      scene.Add(Sprite{
          .asset = std::format("view-tile/map-2/{:02}", n), .palette = std::string{ hour },
                .at = Point{ .x = 32 + (n * TILE), .y = 16 + (row * TILE) }
      });
    });
  }
}

auto AddUnits(Scene& scene) -> void {
  std::ranges::for_each(std::views::iota(0, 8), [&scene](int const n) {
    auto const unit{ std::format("battle-unit/class-{:02}/facing-1", n) };
    scene.Add(Sprite{
        .asset = unit, .at = Point{ .x = 32 + (n * 48), .y = 120 }
    });
    scene.Add(Sprite{
        .asset = unit + "/side-0", .at = Point{ .x = 32 + (n * 48), .y = 150 },
               .flip = render::Flip::HORIZONTAL
    });
  });
}

auto AddPictures(Scene& scene, std::size_t const frame) -> void {
  scene.Add(Sprite{
      .asset = "battle-blast/large", .frame = frame / 8 % 4, .at = Point{ .x = 440, .y = 120 }
  });
  std::ranges::for_each(std::views::iota(0, 4), [&scene](int const n) {
    auto const flip{ n % 2 == 0 ? render::Flip::NONE : render::Flip::HORIZONTAL };
    scene.Add(Sprite{
        .asset = std::format("portrait/{:03}", n), .at = Point{ .x = 32 + (n * 72), .y = 190 },
              .flip = flip
    });
  });
  scene.Add(Sprite{
      .asset = "portrait/004", .at = Point{ .x = 320,     .y = 190      },
           .size = render::Size{ .width = 128, .height = 160 }
  });
  scene.Add(Sprite{
      .asset = "logo/presents", .palette = "main/title", .at = Point{ .x = 32, .y = 290 }
  });
}

auto AddText(Scene& scene, assets::PaletteColours const& day) -> void {
  auto const run{ [](unsigned const first) { return std::views::iota(first, first + 16u) | std::ranges::to<std::vector<render::GlyphIndex>>(); } };
  scene.Add(render::Text{
      .font = std::string{ render::GAME_FONT },
        .glyphs = run(0), .colour = render::Pen(day, 7), .at = Point{ .x = 32, .y = 380 }
  });
  scene.Add(render::Text{
      .font = std::string{ render::GAME_FONT },
        .glyphs = run(16), .colour = render::Pen(day, 14), .at = Point{ .x = 32, .y = 398 }
  });
  scene.Add(render::Text{
      .font = std::string{ render::ENDING_FONT },
        .glyphs = run(0), .colour = render::Pen(day, 2), .at = Point{ .x = 32, .y = 416 }
  });
  scene.Add(render::Text{
      .font = std::string{ render::ENDING_FONT },
        .glyphs = run(16), .colour = render::Pen(day, 5), .at = Point{ .x = 32, .y = 434 }
  });
}

// Full brightness at frame 0, a quarter halfway through the period, full again at its end.
[[nodiscard]] auto FadeLevel(std::size_t const frame) -> std::uint8_t {
  auto const half{ static_cast<int>(FADE_PERIOD / 2) };
  auto const offset{ static_cast<int>(frame % FADE_PERIOD) - half };
  auto const distance{ std::max(offset, -offset) };
  return static_cast<std::uint8_t>(0x40 + (distance * 0xBF / half));
}

}

auto DemoScene(std::size_t const frame, assets::PaletteColours const& day) -> Scene {
  auto scene{ Scene{} };
  scene.Add(render::Fill{ .area = render::LOGICAL_SCREEN, .colour = render::Pen(day, 8) });
  scene.Add(render::Fill{
      .area = render::Rect{ .x = 24, .y = 8, .width = 592, .height = 112 },
        .colour = render::Pen(day, 0)
  });
  AddTiles(scene);
  AddUnits(scene);
  AddPictures(scene, frame);
  scene.Add(render::Line{
      .from = Point{ .x = 24,  .y = 370 },
        .to = Point{ .x = 615, .y = 370 },
        .colour = render::Pen(day, 15)
  });
  AddText(scene, day);
  scene.Tint(render::Dimmed(render::WHITE, FadeLevel(frame)));
  return scene;
}

}
