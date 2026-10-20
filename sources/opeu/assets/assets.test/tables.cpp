// The tables beside the classic pack (packs.embed/classic/tables/): each one, read back, is what the decoders read from
// the shipped file, every record of it; a table written from the decoders' output reads back the same; a malformed
// table is refused. A clone without the pack or the game's files skips what needs them.
#include "packs.hpp"
#include "shipped.hpp"

#include "opeu/assets/code-table.hpp"
#include "opeu/assets/glyph-code-table.hpp"
#include "opeu/assets/glyphs.hpp"
#include "opeu/assets/map-tables.hpp"
#include "opeu/assets/maps.hpp"
#include "opeu/assets/palette-table.hpp"
#include "opeu/assets/palette.hpp"
#include "opeu/assets/scenario-table.hpp"
#include "opeu/assets/scenario.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets {

namespace {

using test::ClassicFile;
using test::Shipped;
using test::ToBytes;

using ScenarioTable             = test::ClassicAgainstShippedTest;
using MapTable                  = test::ClassicAgainstShippedTest;
using MapTableRoundTrip         = test::ShippedTest;
using BattleFieldTable          = test::ClassicAgainstShippedTest;
using BattleFieldTableRoundTrip = test::ShippedTest;
using PaletteTable              = test::CheckedInClassicTest;
using GlyphCodeTable            = test::CheckedInClassicTest;

auto DecodedScenarios(Source const& data) -> ScenarioFile {
  return DecodeScenarioFile(Shipped(data, "SCEDATA")).value();
}

auto DecodedLayers(Source const& data) -> MapLayers {
  auto const grids{ [&data](std::string_view const name) { return DecodeCellGrids(Shipped(data, name)).value(); } };
  return MapLayers{ .cells = grids("MAPINF.DAT"), .tiles = grids("MAPPNO.DAT"), .battle_fields = grids("HEXDISP.DAT") };
}

auto DecodedFields(Source const& data) -> std::vector<BattleField> {
  return DecodeBattleFields(Shipped(data, "HEXMAP.DAT")).value();
}

auto ProgramPalettes() -> std::vector<PaletteRecord> {
  return ClassicPalettes()
         | std::views::transform([](NamedPalette const& palette) { return PaletteRecord{ .name = std::string{ palette.name }, .levels = palette.levels, .use = std::string{ palette.use } }; })
         | std::ranges::to<std::vector>();
}

auto Table(std::string_view const path) -> std::vector<std::byte> {
  return ClassicFile(path);
}

auto PackedCodeTable() -> CodeTable {
  return ReadCodeTable(ClassicFile(CODE_TABLE)).value();
}

}

TEST_F(ScenarioTable, HoldsEveryRecordTheDecoderReads) {
  EXPECT_EQ(ReadScenarioTable(Table(SCENARIO_TABLE), PackedCodeTable()), DecodedScenarios(Data()));
}

TEST_F(ScenarioTable, ReadsBackWhatItWrites) {
  auto const decoded{ DecodedScenarios(Data()) };
  EXPECT_EQ(ReadScenarioTable(ToBytes(ScenarioTableText(decoded, PackedCodeTable())), PackedCodeTable()), decoded);
}

TEST_F(ScenarioTable, WritesSetupZerosKoeiNamesAsUtf8) {
  auto const text{ Table(SCENARIO_TABLE) };
  auto const read{ ReadScenarioTable(text, PackedCodeTable()).value() };
  EXPECT_EQ(read.SetupOf(0).people.front().name, DecodedScenarios(Data()).SetupOf(0).people.front().name);
  EXPECT_FALSE(read.SetupOf(0).people.front().name.empty());
  EXPECT_TRUE(AsText(text).contains(R"("name": "龍施泰特")"));
  EXPECT_FALSE(AsText(text).contains("_koei"));
}

TEST_F(ScenarioTable, KeepsAKoeiNameTheCodeTableCannotNameAsItsBytes) {
  auto file{ DecodedScenarios(Data()) };
  auto setups{ ScenarioFile::Setups{} };
  std::ranges::transform(Indices(setups.size()), setups.begin(), [&file](std::size_t const n) { return file.SetupOf(n); });
  setups.front().people.front().name = "\x81\x40";
  auto const edited{
    ScenarioFile{ {}, {}, setups }
  };
  auto const text{ ScenarioTableText(edited, PackedCodeTable()) };
  EXPECT_TRUE(text.contains(R"("name_koei":"8140")"));
  EXPECT_EQ(ReadScenarioTable(ToBytes(text), PackedCodeTable()), edited);
}

TEST_F(MapTable, HoldsEveryLayerTheDecoderReads) {
  EXPECT_EQ(ReadMapTable(Table(MAP_TABLE)), DecodedLayers(Data()));
}

TEST_F(MapTableRoundTrip, ReadsBackWhatItWrites) {
  auto const decoded{ DecodedLayers(Data()) };
  EXPECT_EQ(ReadMapTable(ToBytes(MapTableText(decoded))), decoded);
}

TEST_F(BattleFieldTable, HoldsEveryFieldTheDecoderReads) {
  EXPECT_EQ(ReadBattleFieldTable(Table(BATTLE_FIELD_TABLE)), DecodedFields(Data()));
}

TEST_F(BattleFieldTableRoundTrip, ReadsBackWhatItWrites) {
  auto const decoded{ DecodedFields(Data()) };
  EXPECT_EQ(ReadBattleFieldTable(ToBytes(BattleFieldTableText(decoded))), decoded);
}

TEST_F(PaletteTable, HoldsEveryPaletteOfTheProgram) {
  EXPECT_EQ(ReadPaletteTable(Table(PALETTE_TABLE)), ProgramPalettes());
}

TEST_F(GlyphCodeTable, HoldsTheEndingFontsCodes) {
  EXPECT_EQ(ReadGlyphCodeTable(Table(GLYPH_CODE_TABLE)), EndingGlyphCodes() | std::ranges::to<std::vector>());
}

TEST(Tables, RefuseAnotherFormat) {
  EXPECT_EQ(ReadGlyphCodeTable(ToBytes(R"({"format": "opeu-tables/0", "glyph_codes": {"ending": []}})")).error().fault, PackFault::BAD_TABLE);
}

TEST(Tables, RefuseAMissingRecord) {
  EXPECT_EQ(ReadGlyphCodeTable(ToBytes(R"({"format": "opeu-tables/1", "glyph_codes": {"ending": [1, 2]}})")).error().fault, PackFault::BAD_TABLE);
}

TEST_F(MapTableRoundTrip, RefusesAValueOutOfRange) {
  auto       text{ MapTableText(DecodedLayers(Data())) };
  auto const at{ text.find(R"("side_0_holds":[[0)") };
  ASSERT_NE(at, std::string::npos);
  text.replace(at, std::string_view{ R"("side_0_holds":[[0)" }.size(), R"("side_0_holds":[[2)");
  EXPECT_EQ(ReadMapTable(ToBytes(text)).error().fault, PackFault::BAD_TABLE);
}

TEST(Tables, RefuseText) {
  EXPECT_EQ(ReadPaletteTable(ToBytes("not json")).error().fault, PackFault::BAD_TABLE);
}

}
