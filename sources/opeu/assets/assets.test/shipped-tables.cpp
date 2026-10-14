// The data files: the scenario file and the map layers.
#include "shipped.hpp"

#include "opeu/assets/maps.hpp"
#include "opeu/assets/scenario.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <map>
#include <ranges>

namespace opeu::assets {

namespace {

using test::Shipped;

auto Grids(std::string_view const file) -> std::array<CellGrid, MAP_COUNT> {
  auto const grids{ DecodeCellGrids(Shipped(file)) };
  EXPECT_TRUE(grids) << file;
  return grids.value_or(std::array<CellGrid, MAP_COUNT>{});
}

}

TEST(ShippedScenarios, HoldTextsConditionsAndSevenSetups) {
  auto const file{ DecodeScenarioFile(Shipped("SCEDATA")) };
  ASSERT_TRUE(file);
  EXPECT_TRUE(file->Description(0).starts_with("  The Russo-German Non-aggression Pact"));
  EXPECT_TRUE(file->Condition(0, 0).starts_with(" 1. Occupy Sedan and"));
  EXPECT_TRUE(file->Condition(0, 1).starts_with(" 1. Defend Sedan."));
  EXPECT_TRUE(
      std::ranges::all_of(std::views::iota(0uz, 6uz), [&](std::size_t const n) { return file->Condition(n, 0).size() < 200 && file->Condition(n, 1).size() < 200 && !file->Description(n).empty(); }));
  EXPECT_EQ(file->SetupOf(0).scenario, 0x80);
  constexpr auto STARTS{ std::array<std::array<unsigned, 3>, 6>{ { { 1940, 5, 10 }, { 1942, 5, 26 }, { 1943, 7, 5 }, { 1944, 6, 6 }, { 1944, 12, 16 }, { 1945, 4, 16 } } } };
  for (auto const [n, start] : std::views::zip(std::views::iota(1uz), STARTS)) {
    auto const& setup{ file->SetupOf(n) };
    EXPECT_EQ(setup.scenario, n - 1);
    EXPECT_EQ((std::array<unsigned, 3>{ setup.year, setup.month, setup.day }), start) << n;
    EXPECT_EQ(setup.people.size(), 40U);
    EXPECT_EQ(setup.elements.size(), 256U);
    EXPECT_EQ(setup.units.size(), 64U);
    EXPECT_EQ(setup.objectives.size(), 21U);
  }
  auto const& france{ file->SetupOf(1) };
  EXPECT_TRUE(std::ranges::contains(france.people, std::string{ "Rundstedt" }, &detail::scenario::Person::name));
  EXPECT_TRUE(std::ranges::contains(france.objectives, std::string{ "San Michel" }, &detail::scenario::Objective::name));
  EXPECT_EQ(france.sides[0].stock, (std::array<unsigned, 3>{ 60000, 60000, 60000 }));
  EXPECT_EQ(france.sides[0].income, (std::array<unsigned, 3>{ 2000, 2000, 2000 }));
  EXPECT_FALSE(DecodeScenarioFile(Bytes{ Shipped("SCEDATA") }.first(100)));
}

TEST(ShippedMapCells, HoldSixFortyByFortyMaps) {
  auto const cells{ Grids("MAPINF.DAT") };
  auto       histogram{ std::map<int, int>{} };
  std::ranges::for_each(cells[0], [&histogram](std::uint8_t const cell) { ++histogram[cell & 0x0F]; });
  EXPECT_EQ(histogram, (std::map<int, int>{
                           { 1,  11  },
                           { 2,  79  },
                           { 3,  239 },
                           { 5,  156 },
                           { 7,  651 },
                           { 8,  283 },
                           { 9,  171 },
                           { 10, 10  }
  }));
  EXPECT_TRUE(std::ranges::contains(cells[1], std::uint8_t{ 0x80 | 2 }) || std::ranges::contains(cells[1], std::uint8_t{ 130 }));
}

TEST(ShippedMapTiles, NameATileOfTheMapsSetPerCell) {
  auto const     tiles{ Grids("MAPPNO.DAT") };
  constexpr auto HIGHEST{
    std::array<std::uint8_t, MAP_COUNT>{ 79, 41, 33, 70, 70, 79 }
  };
  for (auto const& [grid, highest] : std::views::zip(tiles, HIGHEST)) {
    EXPECT_EQ(std::ranges::max(grid), highest);
    EXPECT_LT(std::ranges::max(grid), TILES_PER_MAP);
  }
}

TEST(ShippedBattleSelectors, NameEveryFieldOfTheirMap) {
  auto const     selectors{ Grids("HEXDISP.DAT") };
  constexpr auto FIELDS{
    std::array<std::uint8_t, MAP_COUNT>{ 93, 88, 61, 85, 121, 89 }
  };
  for (auto const& [grid, fields] : std::views::zip(selectors, FIELDS)) {
    EXPECT_EQ(std::ranges::min(grid), 1);
    EXPECT_EQ(std::ranges::max(grid), fields);
  }
}

TEST(ShippedBattleFields, Hold537FieldsOfKnownTerrain) {
  auto const fields{ DecodeBattleFields(Shipped("HEXMAP.DAT")) };
  ASSERT_TRUE(fields);
  EXPECT_EQ(fields->size(), FIELD_COUNT);
  EXPECT_TRUE(std::ranges::all_of(*fields | std::views::join, [](std::uint8_t const cell) { return (cell & 0x3F) <= 42; }));
  EXPECT_EQ(FIRST_FIELD.back() + 89U, FIELD_COUNT);
  EXPECT_EQ(BattleTerrain(5), 17);
  EXPECT_EQ(BattleTerrain(60), 0);
}

}
