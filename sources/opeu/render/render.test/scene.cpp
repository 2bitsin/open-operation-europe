#include "headless.hpp"

#include "opeu/render/atlas-text-renderer.hpp"
#include "opeu/render/baking.hpp"
#include "opeu/render/scene-renderer.hpp"
#include "opeu/render/scene.hpp"

#include <gtest/gtest.h>

#include <format>
#include <ranges>
#include <string>
#include <vector>

namespace opeu::render::detail::scene_test {
namespace {

using test::ExpectGolden;
using test::OpenHeadless;
using test::OpenSample;
using test::PackUnderTest;

class Scenes : public testing::TestWithParam<PackUnderTest> {
public:
  test::Headless    headless{ OpenHeadless() };
  assets::Pack      pack{ OpenSample(GetParam()) };
  TextureCache      textures{ headless.display.Renderer(), pack, 16 };
  AtlasTextRenderer text{ AtlasTextRenderer::Build(headless.display.Renderer(), pack).value() };
  SceneRenderer     scenes{ headless.display, textures, text };

  [[nodiscard]] auto Pen(std::size_t const index) const -> Rgba {
    return render::Pen(test::Needed(pack.Palette("main/day"), "the day palette"), index);
  }

  // A battle corner: terrain at night, a unit, a blast's frames, a frame line and a caption.
  [[nodiscard]] auto Battle() const -> Scene {
    auto scene{ Scene{} };
    scene.Add(Fill{
        .area = Rect{ .x = 0, .y = 0, .width = 160, .height = 96 },
          .colour = Pen(1)
    });
    std::ranges::for_each(std::views::iota(0, 4), [&scene](int const n) {
      scene.Add(Sprite{
          .asset = "view-tile/map-2/17", .palette = n % 2 == 0 ? "main/day" : "main/night", .at = Point{ .x = 24 * n, .y = 0 }
      });
    });
    scene.Add(Sprite{
        .asset = "battle-unit/class-03/facing-1", .at = Point{ .x = 8, .y = 30 }
    });
    scene.Add(Sprite{
        .asset = "battle-unit/class-03/facing-1/side-0", .at = Point{ .x = 56, .y = 30 },
             .flip = Flip::HORIZONTAL
    });
    std::ranges::for_each(std::views::iota(0uz, 4uz), [&scene](std::size_t const n) {
      scene.Add(Sprite{
          .asset = "battle-blast/large", .frame = n, .at = Point{ .x = 100, .y = static_cast<int>(n) * 8 }
      });
    });
    scene.Add(Line{
        .from = Point{ .x = 0,   .y = 60 },
          .to = Point{ .x = 159, .y = 60 },
          .colour = Pen(15)
    });
    scene.Add(Text{
        .font = std::string{ GAME_FONT },
          .glyphs = { 1, 2, 3, 4, 5, 6 },
          .colour = Pen(6), .at = Point{ .x = 4, .y = 66 }
    });
    scene.Add(Sprite{
        .asset = "portrait/118", .at = Point{ .x = 120,    .y = 64      },
             .size = Size{ .width = 32, .height = 40 }
    });
    return scene;
  }

  [[nodiscard]] auto Corner() const -> assets::Texture {
    return headless.display.Read(Rect{ .x = 0, .y = 0, .width = 160, .height = 112 }).value();
  }
};

TEST_P(Scenes, DrawsEveryCommandInOrder) {
  headless.display.Begin();
  ASSERT_TRUE(scenes.Compose(Battle()));
  ExpectGolden(Corner(), GetParam(), "scene");
}

TEST_P(Scenes, FadesByTheScenesTint) {
  auto scene{ Battle() };
  scene.Tint(Dimmed(WHITE, 0x60));
  ASSERT_TRUE(scenes.Show(scene));
  auto const shown{ headless.display.Capture(scene.Tinted()).value() };
  headless.display.Begin();
  ASSERT_TRUE(scenes.Compose(scene));
  auto const faded{ headless.display.Capture(Dimmed(WHITE, 0x60)).value() };
  EXPECT_EQ(shown, faded);
  ExpectGolden(test::Crop(shown, Size{ .width = 160, .height = 112 }), GetParam(), "scene-faded");
}

TEST_P(Scenes, BakesTexturesOncePerAssetAndPalette) {
  headless.display.Begin();
  ASSERT_TRUE(scenes.Compose(Battle()));
  ASSERT_TRUE(scenes.Compose(Battle()));
  EXPECT_EQ(textures.Statistics().misses, 9u); // two tile palettes, two units, four blast frames, the portrait
  EXPECT_EQ(textures.Statistics().hits, (2u * 11u) - 9u);
}

TEST_P(Scenes, LaysTilesOutInRows) {
  auto const ids{ std::views::iota(10, 19) | std::views::transform([](int const n) { return std::format("view-tile/map-2/{}", n); }) | std::ranges::to<std::vector>() };
  auto       scene{ Scene{} };
  scene.Add(Tiles{
      .assets = ids, .columns = 3, .cell = Size{ .width = 24, .height = 24 },
              .palette = "main/dusk", .at = Point{ .x = 4,      .y = 4       }
  });
  scene.Add(Tiles{
      .assets = ids, .columns = 9, .cell = Size{ .width = 24, .height = 24 },
              .palette = "main/night", .at = Point{ .x = 4,      .y = 80      }
  });
  headless.display.Begin();
  ASSERT_TRUE(scenes.Compose(scene));
  EXPECT_EQ(textures.Statistics().misses, 2u * 9u);
  ExpectGolden(headless.display.Read(Rect{ .x = 0, .y = 0, .width = 224, .height = 108 }).value(), GetParam(), "tiles");
}

TEST_P(Scenes, StopsAtACommandItCannotDraw) {
  auto scene{ Scene{} };
  scene.Add(Sprite{ .asset = "battle-blast/large", .frame = 9 });
  EXPECT_FALSE(scenes.Compose(scene).has_value());
  EXPECT_FALSE(scenes
                   .Compose(Scene{
  }
                                .Add(Tiles{ .assets = { "view-tile/map-2/17", "view-tile/map-9/00" }, .columns = 2 }))
                   .has_value());
}

INSTANTIATE_TEST_SUITE_P(Packs, Scenes, testing::ValuesIn(test::PACKS), test::PackName);

}

}
