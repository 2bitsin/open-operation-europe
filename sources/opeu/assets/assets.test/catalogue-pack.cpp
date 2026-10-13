// The decoders' pack: every asset and variant, the animations, fonts and palettes. That it equals a built pack entry
// for entry and pixel for pixel is classic-pack.cpp.
#include "packs.hpp"
#include "shipped.hpp"

#include "opeu/assets/catalogue-pack.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

namespace opeu::assets {

namespace {

using test::Embedded;
using test::OpenSample;
using test::SAMPLE_ARCHIVE;

auto Decoders() -> CataloguePack {
  return CataloguePack{ Catalogue{ Embedded() } };
}

}

TEST(CataloguePack, ListsEveryAssetOfTheCatalogueAndTheUnitIconsVariants) {
  auto const ids{ Decoders().Ids() };
  EXPECT_EQ(ids.size(), 3'218U + 44U);
  EXPECT_TRUE(std::ranges::is_sorted(ids));
  EXPECT_TRUE(std::ranges::contains(ids, "battle-unit/class-03/facing-1/side-0"));
  EXPECT_TRUE(std::ranges::contains(ids, "text-glyph/065"));
}

TEST(CataloguePack, KnowsTheAnimationsFontsPalettesAndScale) {
  auto const decoders{ Decoders() };
  auto const sample{ OpenSample(SAMPLE_ARCHIVE) };
  EXPECT_EQ(decoders.Frames("battle-blast/large"), sample.Frames("battle-blast/large"));
  EXPECT_EQ(decoders.FontNames(), sample.FontNames());
  auto const text{ decoders.FontNamed("text-glyph") };
  ASSERT_TRUE(text);
  EXPECT_EQ(text->first_code, 0x20U);
  EXPECT_EQ(text->glyphs.size(), 96U);
  EXPECT_EQ(text->glyphs[0x41 - 0x20], "text-glyph/065");
  EXPECT_EQ(decoders.FontNamed("game-glyph")->glyphs.size(), 1080U);
  EXPECT_EQ(decoders.Palette("open/047"), sample.Palette("open/047"));
  EXPECT_EQ(decoders.Scale(), 1U);
}

TEST(CataloguePack, SaysAnUnknownIdIsUnknown) {
  auto const decoders{ Decoders() };
  EXPECT_FALSE(decoders.Find("portrait/120"));
  EXPECT_EQ(decoders.Load("portrait/120").error().fault, PackFault::UNKNOWN_ASSET);
  EXPECT_FALSE(decoders.Frames("battle-blast/huge"));
  EXPECT_FALSE(decoders.FontNamed("kana"));
}

}
