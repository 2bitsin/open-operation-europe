#include "headless.hpp"

#include "opeu/render/texture-cache.hpp"

#include <gtest/gtest.h>

namespace opeu::render::detail::texture_cache_test {
using namespace std::string_literals;
using namespace std::string_view_literals;
namespace {

using test::ExpectGolden;
using test::OpenHeadless;
using test::OpenSample;

constexpr auto TILE{ "view-tile/map-2/17"sv };
constexpr auto PORTRAIT{ "portrait/118"sv };
constexpr auto UNIT{ "battle-unit/class-03/facing-1"sv };

[[nodiscard]] auto Key(std::string_view const asset, std::string_view const palette = {}) -> TextureKey {
  return TextureKey{ .asset = std::string{ asset }, .palette = std::string{ palette } };
}

// The texture drawn alone at the frame's corner, read back at its size.
[[nodiscard]] auto Shown(Display& display, Baked const& baked) -> assets::Texture {
  display.Begin();
  display.Draw(baked.texture, Blit{ .destination = At(Point{}, baked.size) });
  return display.Read(At(Point{}, baked.size)).value();
}

class TextureCaches : public test::SampleTest {
public:
  [[nodiscard]] auto Cache(std::size_t const capacity) const -> TextureCache {
    return TextureCache{ headless.display.Renderer(), pack, capacity };
  }

  test::Headless headless{ OpenHeadless() };
  assets::Pack   pack{ OpenSample(GetParam()) };
};

TEST_P(TextureCaches, BakeAnHourColouredTileInEachPalette) {
  auto       cache{ Cache(8) };
  auto const day{ Shown(headless.display, *cache.Fetch(Key(TILE, "main/day")).value()) };
  auto const night{ Shown(headless.display, *cache.Fetch(Key(TILE, "main/night")).value()) };
  EXPECT_NE(day, night);
  EXPECT_EQ(day, Shown(headless.display, *cache.Fetch(Key(TILE)).value())); // the pack's own image is the day's
  ExpectGolden(day, GetParam(), "tile-day");
  ExpectGolden(night, GetParam(), "tile-night");
}

TEST_P(TextureCaches, ShowAnImageOnlyAssetAsThePackHoldsIt) {
  auto       cache{ Cache(8) };
  auto const own{ Shown(headless.display, *cache.Fetch(Key(PORTRAIT)).value()) };
  EXPECT_EQ(own, Shown(headless.display, *cache.Fetch(Key(PORTRAIT, "main/night")).value()));
  EXPECT_EQ(own, pack.Load(PORTRAIT)->texture);
}

TEST_P(TextureCaches, CountHitsAndMisses) {
  auto       cache{ Cache(8) };
  auto const first{ cache.Fetch(Key(UNIT)).value() };
  auto const again{ cache.Fetch(Key(UNIT)).value() };
  EXPECT_EQ(first, again);
  EXPECT_EQ(first->size, (Size{ .width = 40, .height = 26 })); // the game's size, whatever the pack's scale
  EXPECT_EQ(cache.Statistics(), (CacheStatistics{ .hits = 1, .misses = 1, .evictions = 0 }));
}

TEST_P(TextureCaches, EvictTheLeastRecentlyUsed) {
  auto       cache{ Cache(2) };
  auto const held{ cache.Fetch(Key(TILE, "main/day")).value() };
  ASSERT_TRUE(cache.Fetch(Key(TILE, "main/night")));
  ASSERT_TRUE(cache.Fetch(Key(TILE, "main/day")));
  ASSERT_TRUE(cache.Fetch(Key(PORTRAIT)));
  EXPECT_TRUE(cache.Holds(Key(TILE, "main/day")));
  EXPECT_FALSE(cache.Holds(Key(TILE, "main/night")));
  EXPECT_TRUE(cache.Holds(Key(PORTRAIT)));
  EXPECT_EQ(cache.Statistics(), (CacheStatistics{ .hits = 1, .misses = 3, .evictions = 1 }));
  ASSERT_TRUE(cache.Fetch(Key(TILE, "main/night")));
  EXPECT_FALSE(cache.Holds(Key(TILE, "main/day")));
  EXPECT_EQ(held->size, (Size{ .width = 24, .height = 24 })); // a texture let go stays valid while held
}

TEST_P(TextureCaches, ReportAnUnknownAssetOrPalette) {
  auto cache{ Cache(2) };
  EXPECT_FALSE(cache.Fetch(Key("portrait/999")).has_value());
  EXPECT_FALSE(cache.Fetch(Key(TILE, "main/noon")).has_value());
  EXPECT_EQ(cache.Statistics().evictions, 0u);
}

INSTANTIATE_TEST_SUITE_P(Packs, TextureCaches, testing::ValuesIn(test::PACKS), test::PackName);

}

}
