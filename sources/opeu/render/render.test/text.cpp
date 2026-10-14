#include "headless.hpp"

#include "opeu/render/atlas-text-renderer.hpp"
#include "opeu/render/baking.hpp"

#include <gtest/gtest.h>

#include <ranges>
#include <string>
#include <vector>

namespace opeu::render::detail::text_test {
namespace {

using test::ExpectGolden;
using test::OpenHeadless;
using test::OpenSample;
using test::PackUnderTest;

[[nodiscard]] auto Glyphs(GlyphIndex const first, GlyphIndex const count) -> std::vector<GlyphIndex> {
  return std::views::iota(first, first + count) | std::ranges::to<std::vector>();
}

TEST(FontAtlas, HoldsEveryGlyphOfTheFontInCells) {
  auto const headless{ OpenHeadless() };
  auto const pack{ OpenSample(test::CLASSIC) };
  auto const atlas{ FontAtlas::Build(headless.display.Renderer(), pack, GAME_FONT).value() };
  EXPECT_EQ(atlas.Metrics().count, 24u); // the sample's glyphs
  EXPECT_EQ(atlas.Metrics().cell, (Size{ .width = 16, .height = 14 }));
  EXPECT_EQ(atlas.Area(0), (Rect{ .x = 0, .y = 0, .width = 16, .height = 14 }));
  EXPECT_EQ(atlas.Area(23), (Rect{ .x = 23 * 16, .y = 0, .width = 16, .height = 14 })); // 24 glyphs fill part of one row
  EXPECT_FALSE(atlas.Area(24).has_value());
  EXPECT_EQ(atlas.Atlas().Dimensions(), (Size{ .width = FontAtlas::COLUMNS * 16, .height = 14 }));
}

TEST(FontAtlas, KnowsTheTextFontsCodesAndCells) {
  auto const headless{ OpenHeadless() };
  auto const pack{ OpenSample(test::CLASSIC) };
  auto const atlas{ FontAtlas::Build(headless.display.Renderer(), pack, TEXT_FONT).value() };
  EXPECT_EQ(atlas.Metrics(), (FontMetrics{
                                 .cell = Size{ .width = 8, .height = 16 },
                                   .slot = Size{ .width = 8, .height = 16 },
                                   .count = 96, .first_code = 0x20
  }));
  EXPECT_EQ(atlas.Atlas().Dimensions(), (Size{ .width = FontAtlas::COLUMNS * 8, .height = 3 * 16 }));
  EXPECT_EQ(GlyphsOf("A~", atlas.Metrics().first_code), (std::vector<GlyphIndex>{ 0x41 - 0x20, 0x7E - 0x20 }));
}

TEST(FontAtlas, ReportsAnUnknownFont) {
  auto const headless{ OpenHeadless() };
  auto const pack{ OpenSample(test::CLASSIC) };
  EXPECT_FALSE(FontAtlas::Build(headless.display.Renderer(), pack, "kana").has_value());
}

// A font and the pen its glyphs are drawn in.
struct Inked {
  std::string_view font;
  std::size_t      pen{};
};

class AtlasText : public testing::TestWithParam<PackUnderTest> {
public:
  [[nodiscard]] auto Pen(std::size_t const index) const -> Rgba {
    return render::Pen(test::Needed(pack.Palette("main/day"), "the day palette"), index);
  }
  [[nodiscard]] auto Run(Inked const inked, std::vector<GlyphIndex> glyphs, Point const at) const -> TextRun {
    return TextRun{ .font = std::string{ inked.font }, .glyphs = std::move(glyphs), .colour = Pen(inked.pen), .at = at };
  }
  // A line of the text font, its codes turned into glyph numbers.
  [[nodiscard]] auto Line(std::string_view const line, Point const at) const -> TextRun {
    return Run(Inked{ .font = TEXT_FONT, .pen = 15 }, GlyphsOf(line, text.Font(TEXT_FONT)->get().Metrics().first_code), at);
  }

  test::Headless    headless{ OpenHeadless() };
  assets::Pack      pack{ OpenSample(GetParam()) };
  AtlasTextRenderer text{ AtlasTextRenderer::Build(headless.display.Renderer(), pack).value() };
};

TEST_P(AtlasText, DrawsEachShippedFontInPenColours) {
  auto& display{ headless.display };
  display.Begin();
  ASSERT_TRUE(text.Draw(display, Run(Inked{ .font = GAME_FONT, .pen = 7 }, Glyphs(0, 12), Point{ .x = 0, .y = 0 })));
  ASSERT_TRUE(text.Draw(display, Run(Inked{ .font = GAME_FONT, .pen = 14 }, Glyphs(12, 12), Point{ .x = 0, .y = 16 })));
  ASSERT_TRUE(text.Draw(display, Run(Inked{ .font = ENDING_FONT, .pen = 2 }, Glyphs(0, 12), Point{ .x = 0, .y = 32 })));
  ASSERT_TRUE(text.Draw(display, Run(Inked{ .font = ENDING_FONT, .pen = 5 }, Glyphs(12, 12), Point{ .x = 0, .y = 48 })));
  ExpectGolden(display.Read(Rect{ .x = 0, .y = 0, .width = 192, .height = 62 }).value(), GetParam(), "text-fonts");
}

TEST_P(AtlasText, DrawsTheGamesTextOnPaperAndWithout) {
  auto&      display{ headless.display };
  auto const paper{ Line("Operation Europe", Point{ .x = 4, .y = 2 }) };
  display.Begin();
  display.Fill(Rect{ .x = 0, .y = 0, .width = 136, .height = 40 }, Pen(1));
  ASSERT_TRUE(text.Draw(display, TextRun{ .font = paper.font, .glyphs = paper.glyphs, .colour = Pen(15), .at = paper.at, .paper = Pen(8) }));
  ASSERT_TRUE(text.Draw(display, Line("1940-45 {~}", Point{ .x = 4, .y = 20 })));
  ExpectGolden(display.Read(Rect{ .x = 0, .y = 0, .width = 136, .height = 40 }).value(), GetParam(), "text-paper");
}

TEST_P(AtlasText, DrawsTheDigitsTheSidewaysDigitsAndTheMarkers) {
  auto& display{ headless.display };
  display.Begin();
  display.Fill(Rect{ .x = 0, .y = 0, .width = 72, .height = 64 }, Pen(9));
  ASSERT_TRUE(text.Draw(display, Run(Inked{ .font = DIGIT_FONT, .pen = 7 }, Glyphs(0, 11), Point{ .x = 2, .y = 2 })));
  ASSERT_TRUE(text.Draw(display, TextRun{
                                     .font = std::string{ DIGIT_FONT },
                                       .glyphs = { 1, 9, 4, 4 },
                                       .colour = Pen(7), .at = Point{ .x = 2, .y = 10 },
                                       .paper = Pen(0)
  }));
  std::ranges::for_each(std::views::iota(0u, 11u), [&](GlyphIndex const digit) {
    ASSERT_TRUE(text.Draw(display, Run(Inked{ .font = SIDEWAYS_DIGIT_FONT, .pen = 14 }, { digit }, Point{ .x = 2 + static_cast<int>(digit % 6 * 10), .y = 20 + static_cast<int>(digit / 6 * 6) })));
  });
  ASSERT_TRUE(text.Draw(display, Run(Inked{ .font = MARKER_FONT, .pen = 6 }, { 0, 1 }, Point{ .x = 2, .y = 40 })));
  ExpectGolden(display.Read(Rect{ .x = 0, .y = 0, .width = 72, .height = 64 }).value(), GetParam(), "digits");
}

TEST_P(AtlasText, DrawsAGlyphAsThePackHoldsItInItsInk) {
  auto&      display{ headless.display };
  auto const glyph{ pack.Load("game-glyph/0005").value() };
  display.Begin();
  display.Clear(Rgba{ .a = 0 });
  ASSERT_TRUE(text.Draw(display, Run(Inked{ .font = GAME_FONT, .pen = 7 }, { 5 }, Point{ .x = 30, .y = 40 })));
  EXPECT_EQ(display.Read(Rect{ .x = 30, .y = 40, .width = 16, .height = 14 }).value(), glyph.texture);
}

TEST_P(AtlasText, MeasuresARunInCells) {
  EXPECT_EQ(text.Measure(TextRun{ .font = std::string{ ENDING_FONT }, .glyphs = Glyphs(0, 5) }).value(), (Size{ .width = 80, .height = 14 }));
  EXPECT_EQ(text.Measure(Line("Europe", Point{})).value(), (Size{ .width = 48, .height = 16 }));
}

TEST_P(AtlasText, ReportsAGlyphPastTheFontAndAnUnknownFont) {
  headless.display.Begin();
  EXPECT_FALSE(text.Draw(headless.display,
                         TextRun{
                             .font = std::string{ GAME_FONT },
                               .glyphs = { 1, 24 }
  })
                   .has_value());
  EXPECT_FALSE(text.Draw(headless.display, TextRun{ .font = "kana", .glyphs = { 1 } }).has_value());
  EXPECT_FALSE(text.Draw(headless.display, Line("\t", Point{})).has_value()); // below the font's first code
}

INSTANTIATE_TEST_SUITE_P(Packs, AtlasText, testing::ValuesIn(test::PACKS), test::PackName);

}

}
