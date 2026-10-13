// The classic pack against the decoders: every image of the pack is, pixel for pixel, what the catalogue decodes from
// the shipped files. By default the sample in assets.test/packs/ in both its forms; tools/assets/pack.test runs this
// over the whole classic pack, naming it in OPEU_CLASSIC_PACK.
#include "packs.hpp"
#include "shipped.hpp"

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/assets/palette.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <ranges>
#include <string>
#include <vector>

namespace opeu::assets {

namespace {

using test::Embedded;
using test::SAMPLE_ARCHIVE;
using test::SAMPLE_DIRECTORY;

auto PacksUnderTest() -> std::vector<std::filesystem::path> {
  auto const given{ std::getenv("OPEU_CLASSIC_PACK") };
  if (given == nullptr) return { SAMPLE_DIRECTORY, SAMPLE_ARCHIVE };
  return { given };
}

// What the decoders draw for an entry: the asset, or for a side-0 variant the asset in side 0's colours.
auto Decoders(Catalogue const& catalogue, PackedAsset const& entry) -> Decoded<Image> {
  auto const asset{ catalogue.Find(entry.variant_of.empty() ? entry.id : entry.variant_of) };
  if (!asset) return std::unexpected{ DecodeError::OUT_OF_RANGE };
  auto image{ catalogue.Decode(*asset) };
  if (entry.variant == "side-0") return image.transform(SideZeroColours);
  return image;
}

auto DrawnAt(Image const& image, std::size_t const n) -> bool {
  return image.Opaque().empty() || image.Opaque()[n] != 0;
}

// The same drawing: the same size, the same pixels drawn, the same index in each.
auto SameDrawing(Image const& packed, Image const& decoded) -> bool {
  if (packed.Width() != decoded.Width() || packed.Height() != decoded.Height()) return false;
  return std::ranges::all_of(Indices(decoded.Pixels().size()),
                             [&](std::size_t const n) { return DrawnAt(packed, n) == DrawnAt(decoded, n) && (!DrawnAt(decoded, n) || packed.Pixels()[n] == decoded.Pixels()[n]); });
}

auto ExpectAsDecoded(AssetPack const& pack, Catalogue const& catalogue, std::string const& id) -> void {
  auto const loaded{ pack.Load(id) };
  ASSERT_TRUE(loaded) << id << ": " << Describe(loaded.error());
  auto const image{ Decoders(catalogue, loaded->entry) };
  auto const palette{ FindPalette(loaded->entry.palette) };
  ASSERT_TRUE(image && palette) << id;
  EXPECT_EQ(loaded->texture, ToTexture(*image, *palette)) << id;
  EXPECT_EQ(loaded->indexed.has_value(), loaded->entry.recolour.has_value()) << id;
  if (loaded->indexed) {
    EXPECT_TRUE(SameDrawing(*loaded->indexed, *image)) << id;
  }
}

}

TEST(ClassicPack, HoldsEveryImageAsTheDecodersDrawIt) {
  auto const catalogue{ Catalogue{ Embedded() } };
  std::ranges::for_each(PacksUnderTest(), [&](std::filesystem::path const& path) {
    auto const pack{ OpenPack(path) };
    ASSERT_TRUE(pack) << path << ": " << Describe(pack.error());
    std::ranges::for_each(pack->Ids(), [&](std::string const& id) { ExpectAsDecoded(*pack, catalogue, id); });
  });
}

TEST(ClassicPack, HoldsTheGamesPalettes) {
  std::ranges::for_each(PacksUnderTest(), [](std::filesystem::path const& path) {
    auto const pack{ OpenPack(path) };
    ASSERT_TRUE(pack) << path;
    EXPECT_TRUE(std::ranges::all_of(ClassicPalettes(), [&](NamedPalette const& palette) { return pack->Palette(palette.name) == Colours(palette.levels); }));
  });
}

}
