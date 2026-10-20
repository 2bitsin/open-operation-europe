// The source layer beside the frame of colour numbers (source-layer.hpp): an image of the pack names the cells it wholly
// covers, any other write makes the cells it touches raw, a copy moves the cells it moves whole, and an XOR undone by
// the same XOR puts its cells back only if nothing drew there in between.
#include "display-differential.hpp"

#include "opeu/render/source-layer.hpp"

#include <gtest/gtest.h>

#include <format>
#include <string>
#include <string_view>

namespace opeu::render::detail::source_layer_test {
namespace {

using SourceLayerOverShippedTiles = test::ClassicSampleTest;

using test::Shipped;

constexpr auto TILE{ "view-tile/map-2/10" };

// The name a cell shows, "raw" for raw numbers.
[[nodiscard]] auto Shown(SourceLayer const& layer, Point const cell) -> std::string {
  return std::string{ layer.Name(layer.At(cell).image).value_or("raw") };
}

[[nodiscard]] auto Offset(SourceLayer const& layer, Point const cell) -> std::string {
  auto const offset{ layer.At(cell).offset };
  return std::format("{},{}", offset.x, offset.y);
}

// The grid's rows start at frame line CELL_TOP: row r spans lines CELL_TOP + 8r to CELL_TOP + 8r + 8.
constexpr auto PICTURE{
  Rect{ .x = 12, .y = CELL_TOP + 16, .width = 28, .height = 16 }
};
constexpr auto SQUARE{
  Rect{ .x = 0, .y = CELL_TOP, .width = 32, .height = 32 }
};
constexpr auto CORNER{
  Rect{ .x = 0, .y = CELL_TOP, .width = 16, .height = 16 }
};

auto PlaceSquare(SourceLayer& layer) -> void {
  layer.Place(SQUARE, Placed{
                          .image = layer.Number("picture/a"), .at = { .x = SQUARE.x, .y = SQUARE.y },
                               .colours = 0
  });
}

TEST(SourceLayer, AnImageNamesTheCellsItWhollyCovers) {
  auto layer{ SourceLayer{} };
  layer.Place(PICTURE, Placed{
                           .image = layer.Number("picture/a"), .at = { .x = PICTURE.x, .y = PICTURE.y },
                                .colours = 0
  });
  EXPECT_EQ(Shown(layer, { .x = 1, .y = 2 }), "raw"); // x 8 to 16: touched, not covered
  EXPECT_EQ(Shown(layer, { .x = 2, .y = 2 }), "picture/a");
  EXPECT_EQ(Offset(layer, { .x = 2, .y = 2 }), "4,0");
  EXPECT_EQ(Offset(layer, { .x = 4, .y = 3 }), "20,8");
  EXPECT_EQ(Shown(layer, { .x = 5, .y = 3 }), "raw");
}

TEST(SourceLayer, AWholeCellCopyMovesTheNamesAndAnyOtherMakesThemRaw) {
  auto layer{ SourceLayer{} };
  PlaceSquare(layer);
  layer.Put(layer.Keep(CORNER), Point{ .x = 64, .y = CELL_TOP + 80 });
  EXPECT_EQ(Shown(layer, { .x = 9, .y = 11 }), "picture/a");
  EXPECT_EQ(Offset(layer, { .x = 9, .y = 11 }), "8,8");
  layer.Put(layer.Keep(CORNER), Point{ .x = 64, .y = CELL_TOP + 124 });
  EXPECT_EQ(Shown(layer, { .x = 8, .y = 16 }), "raw");                                                      // moved by half a cell
  layer.Move(Rect{ .x = 0, .y = CELL_TOP, .width = 32, .height = 16 }, Point{ .x = 0, .y = CELL_TOP + 8 }); // over itself
  EXPECT_EQ(Offset(layer, { .x = 1, .y = 1 }), "8,0");
  EXPECT_EQ(Offset(layer, { .x = 1, .y = 2 }), "8,8");
}

TEST(SourceLayer, AnXorUndoneByTheSameXorPutsItsCellsBackUnlessDrawnOver) {
  auto layer{ SourceLayer{} };
  PlaceSquare(layer);
  auto const cursor{ layer.Toggle(CORNER) };
  EXPECT_EQ(Shown(layer, { .x = 1, .y = 1 }), "raw");
  layer.Clear(Rect{ .x = 0, .y = CELL_TOP, .width = 1, .height = 1 });
  layer.Untoggle(cursor);
  EXPECT_EQ(Shown(layer, { .x = 0, .y = 0 }), "raw"); // drawn over while the cursor showed
  EXPECT_EQ(Shown(layer, { .x = 1, .y = 1 }), "picture/a");
}

// The invalidation through the display: a shipped tile drawn as the game draws it, then a corner of it overwritten.
TEST_F(SourceLayerOverShippedTiles, ACornerDrawnOverAShippedTileMakesItsCellRaw) {
  auto  rendered{ test::Rendered{} };
  auto& display{ rendered.Screen() };
  display.EnterGraphics();
  auto const tile{ Shipped().Record(TILE, 3uz * 24 * 4) };
  std::ignore = display.DrawColumns({ .x = 1, .y = 20, .width = 3, .height = 24 }, { .mode = 0, .bytes = tile }); // where map tiles lie
  auto const& layer{ display.Sources() };
  EXPECT_EQ(display.Statistics().recognised, 1U);
  EXPECT_EQ(Shown(layer, { .x = 1, .y = 7 }), TILE); // game line 20 is frame line 60, cell row 7
  EXPECT_EQ(Offset(layer, { .x = 3, .y = 9 }), "16,16");
  std::ignore = display.Box({ .x = 1, .y = 20, .width = 1, .height = 4 }, 9);
  EXPECT_EQ(Shown(layer, { .x = 1, .y = 7 }), "raw");
  EXPECT_EQ(Shown(layer, { .x = 2, .y = 7 }), TILE);
  EXPECT_EQ(Shown(layer, { .x = 1, .y = 8 }), TILE);
  display.Line(
      {
          .from = { .x = 20, .y = 30 },
            .to = { .x = 21, .y = 30 }
  },
      0x000F);
  EXPECT_EQ(Shown(layer, { .x = 2, .y = 8 }), "raw");
}

}

}
