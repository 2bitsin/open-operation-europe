// The program's own glyph tables: the text font, the digits, the sideways digits and the markers, decoded through
// the catalogue like the shipped bitmaps.
#include "shipped.hpp"

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/program-glyphs.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ranges>
#include <string_view>
#include <vector>

namespace opeu::assets {

namespace {

auto GlyphImage(std::string_view const id) -> Image {
  auto const  shipped{ test::ShippedCatalogue{} };
  auto const& catalogue{ shipped.Catalogued() };
  auto const  asset{ catalogue.Find(id) };
  if (!asset) {
    ADD_FAILURE() << "no asset " << id;
    return Image{};
  }
  return catalogue.Decode(*asset).value_or(Image{});
}

auto InkedPixels(Image const& image) -> std::size_t {
  return static_cast<std::size_t>(std::ranges::count(image.Opaque(), std::uint8_t{ 1 }));
}

// The set bits of a table's rows from `at`.
auto SetBits(std::string_view const table, std::size_t const at, std::size_t const rows) -> std::size_t {
  auto const bytes{ ProgramFile(table).value().subspan(at, rows) };
  return std::ranges::fold_left(bytes | std::views::transform([](std::byte const b) { return static_cast<std::size_t>(std::popcount(std::to_integer<std::uint8_t>(b))); }), 0uz, std::plus{});
}

}

TEST(ProgramGlyphs, NameFourTablesAndNothingElse) {
  EXPECT_EQ(ProgramFile(PROGRAM_TEXT_FONT)->size(), TEXT_GLYPHS * TEXT_GLYPH_HEIGHT);
  EXPECT_EQ(ProgramFile(PROGRAM_DIGITS)->size(), DIGIT_GLYPHS * DIGIT_SIZE);
  EXPECT_EQ(ProgramFile(PROGRAM_SIDEWAYS_DIGITS)->size(), DIGIT_GLYPHS * SIDEWAYS_DIGIT_HEIGHT);
  EXPECT_EQ(ProgramFile(PROGRAM_MARKERS)->size(), MARKERS * MARKER_SIZE);
  EXPECT_FALSE(ProgramFile("FACE.DAT"));
}

TEST(ProgramGlyphs, DrawTheTextFontInEightBySixteenCellsByCharacterCode) {
  auto const letter{ GlyphImage("text-glyph/065") };
  EXPECT_EQ(letter.Width(), 8U);
  EXPECT_EQ(letter.Height(), TEXT_GLYPH_HEIGHT);
  EXPECT_EQ(InkedPixels(letter), SetBits(PROGRAM_TEXT_FONT, (0x41 - FIRST_TEXT_CODE) * TEXT_GLYPH_HEIGHT, TEXT_GLYPH_HEIGHT));
  EXPECT_EQ(InkedPixels(GlyphImage("text-glyph/032")), 0U); // the space
}

TEST(ProgramGlyphs, DrawTheDigitsSixPixelsWideAndTheBlankAsAFullCell) {
  auto const seven{ GlyphImage("digit-glyph/07") };
  EXPECT_EQ(seven.Width(), DIGIT_SIZE);
  EXPECT_EQ(seven.Height(), DIGIT_SIZE);
  EXPECT_EQ(InkedPixels(seven), SetBits(PROGRAM_DIGITS, 7 * DIGIT_SIZE, DIGIT_SIZE));
  EXPECT_EQ(InkedPixels(GlyphImage("digit-glyph/10")), DIGIT_SIZE * DIGIT_SIZE);
}

TEST(ProgramGlyphs, DrawTheSidewaysDigitsEightByFive) {
  auto const three{ GlyphImage("sideways-digit-glyph/03") };
  EXPECT_EQ(three.Width(), 8U);
  EXPECT_EQ(three.Height(), SIDEWAYS_DIGIT_HEIGHT);
  EXPECT_EQ(InkedPixels(three), SetBits(PROGRAM_SIDEWAYS_DIGITS, 3 * SIDEWAYS_DIGIT_HEIGHT, SIDEWAYS_DIGIT_HEIGHT));
}

TEST(ProgramGlyphs, DrawTheTwoMarkersEightByEight) {
  auto const flash{ GlyphImage("marker-glyph/1") };
  EXPECT_EQ(flash.Width(), MARKER_SIZE);
  EXPECT_EQ(flash.Height(), MARKER_SIZE);
  EXPECT_EQ(InkedPixels(flash), SetBits(PROGRAM_MARKERS, MARKER_SIZE, MARKER_SIZE));
}

TEST(ProgramGlyphs, AreCataloguedAsGlyphsInInk) {
  auto const  shipped{ test::ShippedCatalogue{} };
  auto const& catalogue{ shipped.Catalogued() };
  auto const  program{ catalogue.Assets() | std::views::filter([](Asset const& asset) { return asset.file.starts_with("program/"); }) | std::ranges::to<std::vector>() };
  EXPECT_EQ(program.size(), TEXT_GLYPHS + DIGIT_GLYPHS + DIGIT_GLYPHS + MARKERS);
  EXPECT_TRUE(std::ranges::all_of(program, [](Asset const& asset) { return asset.role == Role::GLYPH && asset.layout == Layout::BITMAP && asset.group == "glyphs"; }));
}

}
