// The decoders' pack: every asset and variant, the animations, fonts and palettes. That it equals a built pack entry
// for entry and pixel for pixel is classic-pack.cpp. A clone without the game's files skips them.
#include "present.hpp"
#include "shipped.hpp"

#include "opeu/assets/catalogue-pack.hpp"
#include "opeu/assets/palette.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

namespace opeu::assets {

namespace {

using test::Present;

using ShippedCataloguePack = test::ShippedTest;

}

TEST_F(ShippedCataloguePack, ListsEveryAssetOfTheCatalogueAndTheUnitIconsVariants) {
  auto const ids{ Decoders().Ids() };
  EXPECT_EQ(ids.size(), 3'218U + 44U);
  EXPECT_TRUE(std::ranges::is_sorted(ids));
  EXPECT_TRUE(std::ranges::contains(ids, "battle-unit/class-03/facing-1/side-0"));
  EXPECT_TRUE(std::ranges::contains(ids, "text-glyph/065"));
}

TEST_F(ShippedCataloguePack, ListsWhatItFindsWithoutDrawing) {
  auto const& decoders{ Decoders() };
  EXPECT_TRUE(decoders.Lists("battle-unit/class-03/facing-1/side-0"));
  EXPECT_TRUE(decoders.Lists("portrait/118"));
  EXPECT_FALSE(decoders.Lists("portrait/999"));
  EXPECT_EQ(decoders.Lists("dialog/yes"), decoders.Find("dialog/yes").has_value());
}

TEST_F(ShippedCataloguePack, KnowsTheAnimationsFontsPalettesAndScale) {
  auto const& decoders{ Decoders() };
  EXPECT_EQ(decoders.Frames("battle-blast/large"), (std::vector<std::string>{ "battle-blast/large/0", "battle-blast/large/1", "battle-blast/large/2", "battle-blast/large/3" }));
  EXPECT_EQ(decoders.FontNames(), (std::vector<std::string>{ "digit-glyph", "ending-glyph", "game-glyph", "marker-glyph", "sideways-digit-glyph", "text-glyph" }));
  auto const text{ decoders.FontNamed("text-glyph") };
  ASSERT_TRUE(text);
  EXPECT_EQ(Present(text).first_code, 0x20U);
  EXPECT_EQ(Present(text).glyphs.size(), 96U);
  EXPECT_EQ(Present(text).glyphs[0x41 - 0x20], "text-glyph/065");
  EXPECT_EQ(Present(decoders.FontNamed("game-glyph")).glyphs.size(), 1080U);
  auto const opening{ std::ranges::find(ClassicPalettes(), "open/047", &NamedPalette::name) };
  ASSERT_NE(opening, ClassicPalettes().end());
  EXPECT_EQ(decoders.Palette("open/047"), Colours(opening->levels));
  EXPECT_EQ(decoders.Scale(), 1U);
}

TEST_F(ShippedCataloguePack, SaysAnUnknownIdIsUnknown) {
  auto const& decoders{ Decoders() };
  EXPECT_FALSE(decoders.Find("portrait/120"));
  EXPECT_EQ(decoders.Load("portrait/120").error().fault, PackFault::UNKNOWN_ASSET);
  EXPECT_FALSE(decoders.Frames("battle-blast/huge"));
  EXPECT_FALSE(decoders.FontNamed("kana"));
}

}
