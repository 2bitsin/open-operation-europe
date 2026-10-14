// The tables beside the classic pack (packs.embed/classic/tables/): each one, read back, is what the decoders read from
// the shipped file, every record of it; a table written from the decoders' output reads back the same; a malformed
// table is refused.
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

auto DecodedScenarios() -> ScenarioFile {
  return DecodeScenarioFile(Shipped("SCEDATA")).value();
}

auto DecodedLayers() -> MapLayers {
  auto const grids{ [](std::string_view const name) { return DecodeCellGrids(Shipped(name)).value(); } };
  return MapLayers{ .cells = grids("MAPINF.DAT"), .tiles = grids("MAPPNO.DAT"), .battle_fields = grids("HEXDISP.DAT") };
}

auto DecodedFields() -> std::vector<BattleField> {
  return DecodeBattleFields(Shipped("HEXMAP.DAT")).value();
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

TEST(ScenarioTable, HoldsEveryRecordTheDecoderReads) {
  EXPECT_EQ(ReadScenarioTable(Table(SCENARIO_TABLE), PackedCodeTable()), DecodedScenarios());
}

TEST(ScenarioTable, ReadsBackWhatItWrites) {
  auto const decoded{ DecodedScenarios() };
  EXPECT_EQ(ReadScenarioTable(ToBytes(ScenarioTableText(decoded, PackedCodeTable())), PackedCodeTable()), decoded);
}

TEST(ScenarioTable, WritesSetupZerosKoeiNamesAsUtf8) {
  auto const text{ Table(SCENARIO_TABLE) };
  auto const read{ ReadScenarioTable(text, PackedCodeTable()).value() };
  EXPECT_EQ(read.SetupOf(0).people.front().name, DecodedScenarios().SetupOf(0).people.front().name);
  EXPECT_FALSE(read.SetupOf(0).people.front().name.empty());
  EXPECT_TRUE(AsText(text).contains(R"("name": "龍施泰特")"));
  EXPECT_FALSE(AsText(text).contains("_koei"));
}

TEST(ScenarioTable, KeepsAKoeiNameTheCodeTableCannotNameAsItsBytes) {
  auto file{ DecodedScenarios() };
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

TEST(MapTable, HoldsEveryLayerTheDecoderReads) {
  EXPECT_EQ(ReadMapTable(Table(MAP_TABLE)), DecodedLayers());
}

TEST(MapTable, ReadsBackWhatItWrites) {
  auto const decoded{ DecodedLayers() };
  EXPECT_EQ(ReadMapTable(ToBytes(MapTableText(decoded))), decoded);
}

TEST(BattleFieldTable, HoldsEveryFieldTheDecoderReads) {
  EXPECT_EQ(ReadBattleFieldTable(Table(BATTLE_FIELD_TABLE)), DecodedFields());
}

TEST(BattleFieldTable, ReadsBackWhatItWrites) {
  auto const decoded{ DecodedFields() };
  EXPECT_EQ(ReadBattleFieldTable(ToBytes(BattleFieldTableText(decoded))), decoded);
}

TEST(PaletteTable, HoldsEveryPaletteOfTheProgram) {
  EXPECT_EQ(ReadPaletteTable(Table(PALETTE_TABLE)), ProgramPalettes());
}

TEST(GlyphCodeTable, HoldsTheEndingFontsCodes) {
  EXPECT_EQ(ReadGlyphCodeTable(Table(GLYPH_CODE_TABLE)), EndingGlyphCodes() | std::ranges::to<std::vector>());
}

TEST(Tables, AreEmbeddedWithTheClassicPack) {
  auto const files{ EmbeddedPackFiles{ CLASSIC_PACK } };
  EXPECT_EQ(files.Read(MAP_TABLE), Table(MAP_TABLE));
  EXPECT_EQ(files.Read(SCENARIO_TABLE), Table(SCENARIO_TABLE));
}

TEST(Tables, RefuseAnotherFormat) {
  EXPECT_EQ(ReadGlyphCodeTable(ToBytes(R"({"format": "opeu-tables/0", "glyph_codes": {"ending": []}})")).error().fault, PackFault::BAD_TABLE);
}

TEST(Tables, RefuseAMissingRecord) {
  EXPECT_EQ(ReadGlyphCodeTable(ToBytes(R"({"format": "opeu-tables/1", "glyph_codes": {"ending": [1, 2]}})")).error().fault, PackFault::BAD_TABLE);
}

TEST(Tables, RefuseAValueOutOfRange) {
  auto       text{ MapTableText(DecodedLayers()) };
  auto const at{ text.find(R"("side_0_holds":[[0)") };
  ASSERT_NE(at, std::string::npos);
  text.replace(at, std::string_view{ R"("side_0_holds":[[0)" }.size(), R"("side_0_holds":[[2)");
  EXPECT_EQ(ReadMapTable(ToBytes(text)).error().fault, PackFault::BAD_TABLE);
}

TEST(Tables, RefuseText) {
  EXPECT_EQ(ReadPaletteTable(ToBytes("not json")).error().fault, PackFault::BAD_TABLE);
}

}
