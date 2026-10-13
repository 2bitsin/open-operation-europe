#include "headless.hpp"

#include "opeu/render/atlas-text-renderer.hpp"
#include "opeu/render/baking.hpp"

#include <gtest/gtest.h>

#include <ranges>
#include <vector>

namespace opeu::render::detail::text_test {

using test::ExpectGolden;
using test::OpenHeadless;
using test::OpenSample;

[[nodiscard]] auto Glyphs(GlyphIndex const first, GlyphIndex const count) -> std::vector<GlyphIndex> {
  return std::views::iota(first, first + count) | std::ranges::to<std::vector>();
}

TEST(FontAtlas, HoldsEveryGlyphOfTheFontInCells) {
  auto       headless{ OpenHeadless() };
  auto const pack{ OpenSample() };
  auto const atlas{ FontAtlas::Build(headless.display.Renderer(), PackSource{ pack }, GAME_FONT).value() };
  EXPECT_EQ(atlas.Metrics().count, 24u); // the sample's glyphs
  EXPECT_EQ(atlas.Metrics().cell, (Size{ .width = 16, .height = 14 }));
  EXPECT_EQ(atlas.Area(0), (Rect{ .x = 0, .y = 0, .width = 16, .height = 14 }));
  EXPECT_EQ(atlas.Area(23), (Rect{ .x = 23 * 16, .y = 0, .width = 16, .height = 14 })); // 24 glyphs fill part of one row
  EXPECT_FALSE(atlas.Area(24).has_value());
  EXPECT_EQ(atlas.Atlas().Dimensions(), (Size{ .width = FontAtlas::COLUMNS * 16, .height = 14 }));
}

TEST(FontAtlas, ReportsAnUnknownFont) {
  auto       headless{ OpenHeadless() };
  auto const pack{ OpenSample() };
  EXPECT_FALSE(FontAtlas::Build(headless.display.Renderer(), PackSource{ pack }, "kana").has_value());
}

class AtlasText : public testing::Test {
protected:
  test::Headless    _headless{ OpenHeadless() };
  assets::Pack      _pack{ OpenSample() };
  PackSource        _source{ _pack };
  AtlasTextRenderer _text{ AtlasTextRenderer::Build(_headless.display.Renderer(), _source, CLASSIC_FONTS).value() };
};

TEST_F(AtlasText, DrawsEachShippedFontInPenColours) {
  auto const day{ *_pack.Palette("main/day") };
  auto&      display{ _headless.display };
  display.Begin();
  ASSERT_TRUE(_text.Draw(display, TextRun{
                                      .font = std::string{ GAME_FONT },
                                        .glyphs = Glyphs(0, 12), .colour = Pen(day, 7), .at = Point{ .x = 0, .y = 0 }
  }));
  ASSERT_TRUE(_text.Draw(display, TextRun{
                                      .font = std::string{ GAME_FONT },
                                        .glyphs = Glyphs(12, 12), .colour = Pen(day, 14), .at = Point{ .x = 0, .y = 16 }
  }));
  ASSERT_TRUE(_text.Draw(display, TextRun{
                                      .font = std::string{ ENDING_FONT },
                                        .glyphs = Glyphs(0, 12), .colour = Pen(day, 2), .at = Point{ .x = 0, .y = 32 }
  }));
  ASSERT_TRUE(_text.Draw(display, TextRun{
                                      .font = std::string{ ENDING_FONT },
                                        .glyphs = Glyphs(12, 12), .colour = Pen(day, 5), .at = Point{ .x = 0, .y = 48 }
  }));
  auto const lines{ display.Read(Rect{ .x = 0, .y = 0, .width = 192, .height = 62 }).value() };
  ExpectGolden(lines, "text-fonts");
}

TEST_F(AtlasText, DrawsAGlyphAsThePackHoldsItInItsInk) {
  auto&      display{ _headless.display };
  auto const glyph{ _pack.Load("game-glyph/0005").value() };
  display.Begin();
  display.Clear(Rgba{ .a = 0 });
  ASSERT_TRUE(_text.Draw(display, TextRun{
                                      .font = std::string{ GAME_FONT },
                                        .glyphs = { 5 },
                                        .colour = Pen(*_pack.Palette("main/day"), 7), .at = Point{ .x = 30, .y = 40 }
  }));
  EXPECT_EQ(display.Read(Rect{ .x = 30, .y = 40, .width = 16, .height = 14 }).value(), glyph.texture);
}

TEST_F(AtlasText, MeasuresARunInCells) {
  EXPECT_EQ(_text.Measure(TextRun{ .font = std::string{ ENDING_FONT }, .glyphs = Glyphs(0, 5) }).value(), (Size{ .width = 80, .height = 14 }));
}

TEST_F(AtlasText, ReportsAGlyphPastTheFontAndAnUnknownFont) {
  _headless.display.Begin();
  EXPECT_FALSE(_text
                   .Draw(_headless.display,
                         TextRun{
                             .font = std::string{ GAME_FONT },
                               .glyphs = { 1, 24 }
  })
                   .has_value());
  EXPECT_FALSE(_text.Draw(_headless.display, TextRun{ .font = "kana", .glyphs = { 1 } }).has_value());
}

}
