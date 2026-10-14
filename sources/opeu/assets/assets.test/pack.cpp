// A pack loaded from its directory form, its archive form and the executable's embedded files, by id.
#include "packs.hpp"

#include "opeu/assets/pack.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <format>
#include <string>

namespace opeu::assets {

namespace {

using test::ClassicArchive;
using test::ClassicDirectory;
using test::ClassicEmbedded;
using test::ClassicFile;
using test::OpenMemoryPack;
using test::SAMPLE_IDS;
using test::ToBytes;

auto YesImage() -> std::vector<std::byte> {
  return ClassicFile(ClassicDirectory().Find("dialog/yes")->image);
}

// A one-asset pack whose entry says the image is width x height at the given scale.
auto PackStating(std::size_t const width, std::size_t const height, std::string_view const scale) -> Packed<Pack> {
  auto const manifest{ std::format(R"({{"format": "opeu-pack/1", "name": "x", "scale": 1, "palettes": {{}}, "assets": [
    {{"id": "yes", "class": "dialogs", "role": "sprite", "width": {}, "height": {}, {} "palette": "main/day", "palettes": ["main/day"], "mask": false, "image": "yes.png"}}]}})",
                                   width, height, scale) };
  return OpenMemoryPack({
      { "pack.json", ToBytes(manifest) },
      { "yes.png",   YesImage()        }
  });
}

}

TEST(Pack, LoadsTheSameFromItsDirectoryItsArchiveAndTheExecutable) {
  auto const directory{ ClassicDirectory() };
  auto const archive{ ClassicArchive() };
  auto const embedded{ ClassicEmbedded() };
  ASSERT_EQ(directory.Ids(), archive.Ids());
  ASSERT_EQ(directory.Ids(), embedded.Ids());
  EXPECT_EQ(directory.Ids().size(), 3'262U);
  EXPECT_TRUE(std::ranges::all_of(SAMPLE_IDS, [&](std::string_view const id) {
    auto const loaded{ directory.Load(id) };
    return loaded && loaded == archive.Load(id) && loaded == embedded.Load(id);
  }));
}

TEST(Pack, LoadsAnAssetByItsId) {
  auto const portrait{ ClassicEmbedded().Load("portrait/118") };
  ASSERT_TRUE(portrait) << Describe(portrait.error());
  EXPECT_EQ(portrait->entry.role, "portrait");
  EXPECT_EQ(portrait->texture.Width(), 64U);
  EXPECT_EQ(portrait->texture.Height(), 80U);
  EXPECT_EQ(portrait->texture.Pixels().size(), 64U * 80U * 4U);
}

TEST(Pack, NamesEachImageForWhatItShows) {
  auto const pack{ ClassicEmbedded() };
  EXPECT_EQ(pack.Find("portrait/005")->image, "portraits/portrait-005-busch.png");
  EXPECT_EQ(pack.Find("silhouette/012")->image, "silhouettes/silhouette-012-pzkpfw-i.png");
  EXPECT_EQ(pack.Find("event/27")->image, "pictures/event-27-pontoon-crossing.png");
  EXPECT_EQ(pack.Find("battle-unit/class-03/facing-1/side-0")->image, "battle/battle-unit-class-03-facing-1-light-tank-side-0.png");
  EXPECT_EQ(pack.Find("event/27")->description, "event picture: a tank crossing a pontoon bridge");
  EXPECT_EQ(pack.Find("portrait/005")->scale, 1U);
}

TEST(Pack, GivesTheIndexMapExactlyWhereTheGameRecolours) {
  auto const pack{ ClassicDirectory() };
  EXPECT_TRUE(std::ranges::all_of(SAMPLE_IDS, [&](std::string_view const id) {
    auto const loaded{ pack.Load(id) };
    return loaded && loaded->indexed.has_value() == loaded->entry.recolour.has_value();
  }));
  EXPECT_EQ(pack.Load("view-tile/map-2/17")->entry.recolour, "hour");
  EXPECT_EQ(pack.Load("game-glyph/0042")->entry.recolour, "ink");
  EXPECT_EQ(pack.Load("opening/00")->entry.recolour, "scene");
  EXPECT_FALSE(pack.Load("portrait/118")->indexed);
}

TEST(Pack, ListsAUnitIconsSideZeroVariant) {
  auto const variant{ ClassicDirectory().Find("battle-unit/class-03/facing-1/side-0") };
  ASSERT_TRUE(variant);
  EXPECT_EQ(variant->variant_of, "battle-unit/class-03/facing-1");
  EXPECT_EQ(variant->variant, "side-0");
}

TEST(Pack, KnowsAnimationsAndPalettes) {
  auto const pack{ ClassicEmbedded() };
  EXPECT_EQ(pack.Frames("battle-blast/large"), (std::vector<std::string>{ "battle-blast/large/0", "battle-blast/large/1", "battle-blast/large/2", "battle-blast/large/3" }));
  EXPECT_FALSE(pack.Frames("battle-blast/huge"));
  EXPECT_EQ(pack.Palette("main/day"), Colours(FindPalette("main/day")->levels));
  EXPECT_FALSE(pack.Palette("main/noon"));
}

TEST(Pack, KnowsItsFontsAndItsScale) {
  auto const pack{ ClassicEmbedded() };
  EXPECT_EQ(pack.FontNames(), (std::vector<std::string>{ "digit-glyph", "ending-glyph", "game-glyph", "marker-glyph", "sideways-digit-glyph", "text-glyph" }));
  auto const text{ pack.FontNamed("text-glyph") };
  ASSERT_TRUE(text);
  EXPECT_EQ(text->cell_width, 8U);
  EXPECT_EQ(text->cell_height, 16U);
  EXPECT_EQ(text->first_code, 0x20U);
  EXPECT_EQ(text->glyphs.size(), 96U);
  EXPECT_EQ(text->glyphs[0x41 - 0x20], "text-glyph/065");
  EXPECT_EQ(pack.FontNamed("digit-glyph")->cell_width, 6U);
  EXPECT_FALSE(pack.FontNamed("kana"));
  EXPECT_EQ(pack.Scale(), 1U);
}

TEST(Pack, SaysAnUnknownIdIsUnknown) {
  auto const pack{ ClassicEmbedded() };
  EXPECT_FALSE(pack.Find("portrait/120"));
  EXPECT_EQ(pack.Load("portrait/120").error().fault, PackFault::UNKNOWN_ASSET);
}

TEST(Pack, RefusesAnImageOfAnotherSize) {
  ASSERT_TRUE(PackStating(32, 16, "")->Load("yes"));
  EXPECT_EQ(PackStating(32, 17, "")->Load("yes").error().fault, PackFault::BAD_IMAGE);
}

TEST(Pack, ReadsAnEntrysOwnScale) {
  ASSERT_TRUE(PackStating(16, 8, R"("scale": 2,)")->Load("yes"));
  EXPECT_EQ(PackStating(16, 8, R"("scale": 2,)")->Find("yes")->scale, 2U);
  EXPECT_EQ(PackStating(32, 16, R"("scale": 2,)")->Load("yes").error().fault, PackFault::BAD_IMAGE);
  EXPECT_EQ(PackStating(32, 16, R"("scale": 0,)").error().fault, PackFault::BAD_MANIFEST);
}

TEST(Pack, RefusesAFileThatIsNotAPng) {
  auto const portrait{ ClassicDirectory().Find("portrait/118")->image };
  auto const pack{
    OpenMemoryPack({ { "pack.json", ClassicFile("pack.json") }, { portrait, ToBytes("GIF89a") } }
    )
  };
  ASSERT_TRUE(pack);
  EXPECT_EQ(pack->Load("portrait/118").error().fault, PackFault::BAD_IMAGE);
  EXPECT_EQ(pack->Load("dialog/yes").error().fault, PackFault::MISSING_FILE);
}

TEST(Pack, NeedsAManifest) {
  EXPECT_EQ(OpenMemoryPack({}).error().fault, PackFault::MISSING_FILE);
  EXPECT_EQ(OpenPack("assets.test/no-such.opeupack").error().fault, PackFault::MISSING_FILE);
  EXPECT_EQ(OpenEmbeddedPack("no-such-pack").error().fault, PackFault::MISSING_FILE);
}

}
