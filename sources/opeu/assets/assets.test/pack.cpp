// A pack loaded from its directory form and its archive form, by id.
#include "packs.hpp"

#include "opeu/assets/pack.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <format>
#include <string>

namespace opeu::assets {

namespace {

using test::OpenMemoryPack;
using test::OpenSample;
using test::SAMPLE_ARCHIVE;
using test::SAMPLE_DIRECTORY;
using test::SampleFile;
using test::ToBytes;

// A one-asset pack whose entry says the image is width x height.
auto PackStating(std::size_t const width, std::size_t const height) -> Packed<Pack> {
  auto const manifest{ std::format(R"({{"format": "opeu-pack/1", "name": "x", "scale": 1, "palettes": {{}}, "assets": [
    {{"id": "yes", "class": "dialogs", "role": "sprite", "width": {}, "height": {}, "palette": "main/day", "palettes": ["main/day"], "mask": false, "image": "yes.png"}}]}})",
                                   width, height) };
  return OpenMemoryPack({
      { "pack.json", ToBytes(manifest)                    },
      { "yes.png",   SampleFile("dialogs/dialog/yes.png") }
  });
}

}

TEST(Pack, LoadsTheSameFromItsDirectoryAndItsArchive) {
  auto const directory{ OpenSample(SAMPLE_DIRECTORY) };
  auto const archive{ OpenSample(SAMPLE_ARCHIVE) };
  ASSERT_EQ(directory.Ids(), archive.Ids());
  EXPECT_EQ(directory.Ids().size(), 18U);
  EXPECT_TRUE(std::ranges::all_of(directory.Ids(), [&](std::string const& id) { return directory.Load(id) && directory.Load(id) == archive.Load(id); }));
}

TEST(Pack, LoadsAnAssetByItsId) {
  auto const portrait{ OpenSample(SAMPLE_ARCHIVE).Load("portrait/118") };
  ASSERT_TRUE(portrait) << Describe(portrait.error());
  EXPECT_EQ(portrait->entry.role, "portrait");
  EXPECT_EQ(portrait->texture.Width(), 64U);
  EXPECT_EQ(portrait->texture.Height(), 80U);
  EXPECT_EQ(portrait->texture.Pixels().size(), 64U * 80U * 4U);
}

TEST(Pack, GivesTheIndexMapExactlyWhereTheGameRecolours) {
  auto const pack{ OpenSample(SAMPLE_DIRECTORY) };
  EXPECT_TRUE(std::ranges::all_of(pack.Ids(), [&](std::string const& id) {
    auto const loaded{ pack.Load(id) };
    return loaded && loaded->indexed.has_value() == loaded->entry.recolour.has_value();
  }));
  EXPECT_EQ(pack.Load("view-tile/map-2/17")->entry.recolour, "hour");
  EXPECT_EQ(pack.Load("game-glyph/0042")->entry.recolour, "ink");
  EXPECT_EQ(pack.Load("opening/00")->entry.recolour, "scene");
  EXPECT_FALSE(pack.Load("portrait/118")->indexed);
}

TEST(Pack, ListsAUnitIconsSideZeroVariant) {
  auto const variant{ OpenSample(SAMPLE_DIRECTORY).Find("battle-unit/class-03/facing-1/side-0") };
  ASSERT_TRUE(variant);
  EXPECT_EQ(variant->variant_of, "battle-unit/class-03/facing-1");
  EXPECT_EQ(variant->variant, "side-0");
}

TEST(Pack, KnowsAnimationsAndPalettes) {
  auto const pack{ OpenSample(SAMPLE_ARCHIVE) };
  EXPECT_EQ(pack.Listing().Frames("battle-blast/large"), (std::vector<std::string>{ "battle-blast/large/0", "battle-blast/large/1", "battle-blast/large/2", "battle-blast/large/3" }));
  EXPECT_EQ(pack.Palette("main/day"), Colours(FindPalette("main/day")->levels));
  EXPECT_FALSE(pack.Palette("main/noon"));
}

TEST(Pack, SaysAnUnknownIdIsUnknown) {
  auto const pack{ OpenSample(SAMPLE_ARCHIVE) };
  EXPECT_FALSE(pack.Find("portrait/117"));
  EXPECT_EQ(pack.Load("portrait/117").error().fault, PackFault::UNKNOWN_ASSET);
}

TEST(Pack, RefusesAnImageOfAnotherSize) {
  ASSERT_TRUE(PackStating(32, 16)->Load("yes"));
  EXPECT_EQ(PackStating(32, 17)->Load("yes").error().fault, PackFault::BAD_IMAGE);
}

TEST(Pack, RefusesAFileThatIsNotAPng) {
  auto const pack{
    OpenMemoryPack({ { "pack.json", SampleFile("pack.json") }, { "portraits/portrait/118.png", ToBytes("GIF89a") } }
    )
  };
  ASSERT_TRUE(pack);
  EXPECT_EQ(pack->Load("portrait/118").error().fault, PackFault::BAD_IMAGE);
  EXPECT_EQ(pack->Load("dialog/yes").error().fault, PackFault::MISSING_FILE);
}

TEST(Pack, NeedsAManifest) {
  EXPECT_EQ(OpenMemoryPack({}).error().fault, PackFault::MISSING_FILE);
  EXPECT_EQ(OpenPack("assets.test/packs/no-such.opeupack").error().fault, PackFault::MISSING_FILE);
}

}
