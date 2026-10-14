// The generic plane decoder and its encoder (the renderer saves screen rectangles into the game's memory through it),
// and the content index that tells the renderer which shipped asset a run of the game's bytes is.
#include "present.hpp"
#include "shipped.hpp"

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/content-index.hpp"
#include "opeu/assets/fingerprint.hpp"
#include "opeu/assets/planar.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <vector>

namespace opeu::assets {

namespace {

constexpr auto VIEW_TILE{
  RawShape{ .layout = Layout::PLANES_4, .width_bytes = 3, .height = 24 }
};

using test::Present;

[[nodiscard]] auto Noise(std::size_t const count) -> std::vector<std::byte> {
  auto state{ std::uint32_t{ 11 } };
  return std::views::iota(0uz, count) | std::views::transform([&state](std::size_t) {
           state = (state * 1664525U) + 1013904223U;
           return std::byte{ static_cast<std::uint8_t>(state >> 24) };
         })
         | std::ranges::to<std::vector>();
}

auto ExpectRoundTrip(PlaneLayout const& layout) -> void {
  auto const bytes{ Noise(layout.Bytes()) };
  auto const image{ DecodePlanes(bytes, layout) };
  ASSERT_TRUE(image.has_value());
  EXPECT_EQ(image->Width(), layout.width_bytes * 8);
  EXPECT_EQ(image->Height(), layout.height);
  EXPECT_EQ(EncodePlanes(*image, layout), bytes);
}

TEST(PlaneLayouts, EncodeWhatTheyDecode) {
  ExpectRoundTrip(InterleavedLayout(3, 24, 4));
  ExpectRoundTrip(InterleavedLayout(8, 80, 3));
  ExpectRoundTrip(LinePlanesLayout(5, 26, 4));
  ExpectRoundTrip(PlaneBlocksLayout(6, 26, 4));
  ExpectRoundTrip(InterleavedLayout(1, 16, 1));
}

TEST(PlaneLayouts, RefuseTooFewBytes) {
  EXPECT_FALSE(DecodePlanes(Noise(10), InterleavedLayout(3, 24, 4)).has_value());
}

TEST(Fingerprints, AreFnv1a) {
  EXPECT_EQ(FingerprintOf(Bytes{}), 0xCBF29CE484222325ULL);
  auto const a{ std::vector{ std::byte{ 'a' } } };
  EXPECT_EQ(FingerprintOf(a), 0xAF63DC4C8601EC8CULL);
}

class ShippedContent : public ::testing::Test {
protected:
  [[nodiscard]] auto Index() const -> ContentIndex const& {
    return _index;
  }
  [[nodiscard]] auto Catalogued() const -> Catalogue const& {
    return _shipped.Catalogued();
  }

private:
  test::ShippedCatalogue _shipped;
  ContentIndex           _index{ _shipped.Catalogued() };
};

TEST_F(ShippedContent, RecognisesATileFromItsBytes) {
  auto const tile{ Present(Catalogued().Find("view-tile/map-2/10")) };
  auto const bytes{ Catalogued().File(tile.file).value().subspan(tile.at, 3uz * 24 * 4) };
  auto const id{ Index().Recognise(RawKey(VIEW_TILE, bytes)) };
  ASSERT_TRUE(id.has_value());
  EXPECT_EQ(Present(Catalogued().Find(Present(id))).at, tile.at); // the tile, or the first asset with the same bytes
}

TEST_F(ShippedContent, RecognisesNothingTheGameComposed) {
  EXPECT_GT(Index().Size(), 1000U);
  EXPECT_FALSE(Index().Recognise(RawKey(VIEW_TILE, Noise(288))).has_value());
  EXPECT_FALSE(ContentIndex{}.Recognise(RawKey(VIEW_TILE, Noise(288))).has_value());
}

}

}
