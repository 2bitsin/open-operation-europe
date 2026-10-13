#include "headless.hpp"

#include "opeu/render/atlas-text-renderer.hpp"
#include "opeu/render/baking.hpp"
#include "opeu/render/scene-renderer.hpp"
#include "opeu/render/scene.hpp"

#include <gtest/gtest.h>

#include <ranges>
#include <vector>

namespace opeu::render::detail::scene_test {

using test::ExpectGolden;
using test::OpenHeadless;
using test::OpenSample;

class Scenes : public testing::Test {
protected:
  test::Headless    _headless{ OpenHeadless() };
  assets::Pack      _pack{ OpenSample() };
  PackSource        _source{ _pack };
  TextureCache      _textures{ _headless.display.Renderer(), _source, 16 };
  AtlasTextRenderer _text{ AtlasTextRenderer::Build(_headless.display.Renderer(), _source, CLASSIC_FONTS).value() };
  SceneRenderer     _scenes{ _headless.display, _textures, _text };

  [[nodiscard]] auto Pen(std::size_t const index) const -> Rgba {
    return render::Pen(*_pack.Palette("main/day"), index);
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
    return _headless.display.Read(Rect{ .x = 0, .y = 0, .width = 160, .height = 112 }).value();
  }
};

TEST_F(Scenes, DrawsEveryCommandInOrder) {
  _headless.display.Begin();
  ASSERT_TRUE(_scenes.Compose(Battle()));
  ExpectGolden(Corner(), "scene");
}

TEST_F(Scenes, FadesByTheScenesTint) {
  auto scene{ Battle() };
  scene.Tint(Dimmed(WHITE, 0x60));
  ASSERT_TRUE(_scenes.Show(scene));
  auto const shown{ _headless.display.Capture(scene.Tinted()).value() };
  _headless.display.Begin();
  ASSERT_TRUE(_scenes.Compose(scene));
  auto const faded{ _headless.display.Capture(Dimmed(WHITE, 0x60)).value() };
  EXPECT_EQ(shown, faded);
  ExpectGolden(test::Crop(shown, Size{ .width = 160, .height = 112 }), "scene-faded");
}

TEST_F(Scenes, BakesTexturesOncePerAssetAndPalette) {
  _headless.display.Begin();
  ASSERT_TRUE(_scenes.Compose(Battle()));
  ASSERT_TRUE(_scenes.Compose(Battle()));
  EXPECT_EQ(_textures.Statistics().misses, 9u); // two tile palettes, two units, four blast frames, the portrait
  EXPECT_EQ(_textures.Statistics().hits, (2u * 11u) - 9u);
}

TEST_F(Scenes, StopsAtACommandItCannotDraw) {
  auto scene{ Scene{} };
  scene.Add(Sprite{ .asset = "battle-blast/large", .frame = 9 });
  EXPECT_FALSE(_scenes.Compose(scene).has_value());
}

}
