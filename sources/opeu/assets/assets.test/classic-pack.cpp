// The classic pack against the decoders: every entry and image of the checked-in pack, as a directory and embedded,
// is what the decoders' pack (CataloguePack) draws from the shipped files, pixel for pixel, and the pack holds every
// id, animation and font the decoders know; an archive made of the sample's files reads the same.
#include "packs.hpp"
#include "shipped.hpp"

#include "opeu/assets/catalogue-pack.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/assets/palette.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <span>
#include <string>
#include <vector>

namespace opeu::assets {
using namespace std::string_literals;
using namespace std::string_view_literals;
namespace {

using test::ClassicArchive;
using test::ClassicDirectory;
using test::ClassicEmbedded;

using test::SAMPLE_IDS;

// What the decoders cannot know: the readable file names and descriptions pack.py gives the images.
auto WithoutNaming(LoadedAsset loaded) -> LoadedAsset {
  loaded.entry.image.clear();
  loaded.entry.indexed = loaded.entry.indexed.transform([](std::string const&) { return std::string{}; });
  loaded.entry.description.clear();
  return loaded;
}

// The whole pack's two forms: the checked-in directory and the files compiled into the executable.
auto WholePacks() -> std::vector<Pack> {
  auto packs{ std::vector<Pack>{} };
  packs.push_back(ClassicDirectory());
  packs.push_back(ClassicEmbedded());
  return packs;
}

// Each image of `ids` loads from `pack` as the decoders draw it.
auto ExpectDrawnAsTheDecoders(Pack const& pack, std::span<std::string const> const ids) -> void {
  auto const  shipped{ test::ShippedPack{} };
  auto const& decoders{ shipped.Decoders() };
  std::ranges::for_each(ids, [&](std::string const& id) {
    auto const loaded{ pack.Load(id) };
    ASSERT_TRUE(loaded) << id << ": " << Describe(loaded.error());
    EXPECT_EQ(WithoutNaming(*loaded), WithoutNaming(decoders.Load(id).value())) << id;
  });
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

}

TEST(ClassicPack, HoldsEveryImageAsTheDecodersDrawIt) {
  std::ranges::for_each(WholePacks(), [](Pack const& pack) { ExpectDrawnAsTheDecoders(pack, pack.Ids()); });
}

TEST(ClassicPack, ListsEveryIdAnimationAndFontTheDecodersKnow) {
  auto const shipped{ test::ShippedPack{} };
  std::ranges::for_each(WholePacks(), [&shipped](Pack const& pack) { ExpectComplete(pack, shipped.Decoders()); });
}

TEST(ClassicPack, ReadsTheSampleFromAnArchive) {
  ExpectDrawnAsTheDecoders(ClassicArchive(), SAMPLE_IDS | std::ranges::to<std::vector<std::string>>());
}

TEST(ClassicPack, HoldsTheGamesPalettes) {
  auto packs{ WholePacks() };
  packs.push_back(ClassicArchive());
  std::ranges::for_each(
      packs, [](Pack const& pack) { EXPECT_TRUE(std::ranges::all_of(ClassicPalettes(), [&](NamedPalette const& palette) { return pack.Palette(palette.name) == Colours(palette.levels); })); });
}

}
