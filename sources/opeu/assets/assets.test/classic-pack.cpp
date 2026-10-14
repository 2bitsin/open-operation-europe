// The classic pack against the decoders: every entry and image of the pack is what the decoders' pack (CataloguePack)
// draws from the shipped files, pixel for pixel. By default the sample's ids in the checked-in pack's three forms
// (directory, archive, embedded); tools/assets/pack.test runs this over a whole pack, naming it in OPEU_CLASSIC_PACK,
// and then every id is compared and the pack must also hold every id, animation and font the decoders know.
#include "packs.hpp"
#include "shipped.hpp"

#include "opeu/assets/catalogue-pack.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/assets/palette.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

namespace opeu::assets {

namespace {

using test::ClassicArchive;
using test::ClassicDirectory;
using test::ClassicEmbedded;
using test::SAMPLE_IDS;

auto WholePackGiven() -> std::optional<std::string> {
  auto const given{ std::getenv("OPEU_CLASSIC_PACK") }; // NOLINT(concurrency-mt-unsafe): read before any test thread starts
  if (given == nullptr) return std::nullopt;
  return std::string{ given };
}

auto PacksUnderTest() -> std::vector<Pack> {
  auto packs{ std::vector<Pack>{} };
  if (!WholePackGiven()) {
    packs.push_back(ClassicDirectory());
    packs.push_back(ClassicArchive());
    packs.push_back(ClassicEmbedded());
    return packs;
  }
  auto given{ OpenPack(*WholePackGiven()) };
  EXPECT_TRUE(given) << Describe(given.error());
  if (given) packs.push_back(*std::move(given));
  return packs;
}

// The ids compared: every one of a whole pack, the sample's otherwise.
auto IdsUnderTest(Pack const& pack) -> std::vector<std::string> {
  if (WholePackGiven()) return pack.Ids();
  return SAMPLE_IDS | std::ranges::to<std::vector<std::string>>();
}

// The whole pack lists what the decoders list: every id, every animation's frames, every font.
auto ExpectComplete(AssetPack const& pack, AssetPack const& decoders) -> void {
  EXPECT_EQ(pack.Ids(), decoders.Ids());
  EXPECT_EQ(pack.FontNames(), decoders.FontNames());
  std::ranges::for_each(decoders.FontNames(), [&](std::string const& name) { EXPECT_EQ(pack.FontNamed(name), decoders.FontNamed(name)) << name; });
  std::ranges::for_each(decoders.Ids(), [&](std::string const& id) {
    auto const entry{ decoders.Find(id) };
    if (!entry || entry->animation.empty()) return;
    EXPECT_EQ(pack.Frames(entry->animation), decoders.Frames(entry->animation)) << entry->animation;
  });
}

// What the decoders cannot know: the readable file names and descriptions pack.py gives the images.
auto WithoutNaming(LoadedAsset loaded) -> LoadedAsset {
  loaded.entry.image.clear();
  loaded.entry.indexed = loaded.entry.indexed.transform([](std::string const&) { return std::string{}; });
  loaded.entry.description.clear();
  return loaded;
}

}

TEST(ClassicPack, HoldsEveryImageAsTheDecodersDrawIt) {
  auto const  shipped{ test::ShippedPack{} };
  auto const& decoders{ shipped.pack };
  std::ranges::for_each(PacksUnderTest(), [&](Pack const& pack) {
    std::ranges::for_each(IdsUnderTest(pack), [&](std::string const& id) {
      auto const loaded{ pack.Load(id) };
      ASSERT_TRUE(loaded) << id << ": " << Describe(loaded.error());
      EXPECT_EQ(WithoutNaming(*loaded), WithoutNaming(decoders.Load(id).value())) << id;
    });
    if (WholePackGiven()) ExpectComplete(pack, decoders);
  });
}

TEST(ClassicPack, HoldsTheGamesPalettes) {
  std::ranges::for_each(PacksUnderTest(), [](Pack const& pack) {
    EXPECT_TRUE(std::ranges::all_of(ClassicPalettes(), [&](NamedPalette const& palette) { return pack.Palette(palette.name) == Colours(palette.levels); }));
  });
}

}
