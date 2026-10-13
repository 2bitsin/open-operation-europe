#include "opeu/game/common/data-segment.hpp"
#include "opeu/game/common/game-state.hpp"
#include "opeu/game/units/aggpct.hpp"
#include "opeu/game/units/capacity.hpp"
#include "opeu/game/units/objpct.hpp"
#include "opeu/game/units/resagg.hpp"
#include "opeu/game/units/stats.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <ranges>
#include <span>
#include <utility>

namespace opeu::game::detail::resources_test {
namespace {

constexpr std::uint8_t INFANTRY_TYPE{ 1 };
constexpr std::uint8_t TANK_TYPE{ 2 };

// A zeroed data segment with two element types: infantry (category 0) and tanks (category 3), each carrying ammunition,
// fuel and food by quantity.
class Fixture {
public:
  Fixture() : _bytes{ std::make_unique<std::array<std::uint8_t, SEGMENT_SIZE>>() }, _state{ SegmentBytes{ *_bytes } } {
    auto& data{ Data() };
    data.element_types[INFANTRY_TYPE].category      = 0;
    data.element_types[INFANTRY_TYPE].fuel_capacity = 1;
    data.element_types[TANK_TYPE].category          = 3;
    data.element_types[TANK_TYPE].fuel_capacity     = 10;
    data.ammunition_capacities[0]                   = 2;
    data.ammunition_capacities[3]                   = 5;
    data.food_capacities[0]                         = 3;
    data.food_capacities[3]                         = 1;
  }

  [[nodiscard]] auto Data() -> DataSegment& {
    return _state.Data();
  }

  // Gives unit `index` (side 0, division 0 onwards) one element per entry, chained in order, and marks it ready.
  auto Field(std::size_t index, std::initializer_list<std::pair<std::uint8_t, std::uint16_t>> elements) -> std::uint16_t {
    auto&      data{ Data() };
    auto const chain{ std::span{ data.elements }.subspan(_next_element, elements.size()) };
    _next_element += elements.size();
    for (auto&& [element, spec] : std::views::zip(chain, elements)) {
      element.type     = spec.first;
      element.quantity = spec.second;
    }
    for (auto&& [element, following] : std::views::zip(chain, chain | std::views::drop(1))) element.next = Near(data, following);
    auto& unit{ data.units[index] };
    unit.first  = Near(data, chain.front());
    unit.flags |= Bits(UnitFlag::READY);
    return Near(data, unit);
  }

private:
  std::unique_ptr<std::array<std::uint8_t, SEGMENT_SIZE>> _bytes;
  GameState                                               _state;
  std::size_t                                             _next_element{ 0 };
};
} // namespace

TEST(UnitCapacities, SumEachElementsQuantityTimesItsTypesWeights) {
  Fixture    fixture;
  auto const unit{
    fixture.Field(0, { { INFANTRY_TYPE, 100 }, { TANK_TYPE, 4 } }
     )
  };
  auto const capacities{ UnitCapacities(fixture.Data(), unit) };
  EXPECT_EQ(capacities.ammunition, 100 * 2 + 4 * 5);
  EXPECT_EQ(capacities.fuel, 100 * 1 + 4 * 10);
  EXPECT_EQ(capacities.food, 100 * 3 + 4 * 1);
}

TEST(UnitCapacities, WrapAt16Bits) {
  Fixture    fixture;
  auto const unit{
    fixture.Field(0, { { INFANTRY_TYPE, 40000 }, { INFANTRY_TYPE, 1 } }
     )
  };
  EXPECT_EQ(UnitCapacities(fixture.Data(), unit).ammunition, static_cast<std::uint16_t>(40000 * 2) + 2);
}

TEST(UnitQuantity, CountsTheElementsOfACategory) {
  Fixture    fixture;
  auto const unit{
    fixture.Field(0, { { INFANTRY_TYPE, 100 }, { TANK_TYPE, 4 }, { TANK_TYPE, 6 } }
     )
  };
  auto const tanks{ UnitQuantity(fixture.Data(), unit, 3) };
  EXPECT_EQ(tanks.total, 10);
  EXPECT_EQ(tanks.count, 2);
  auto const all{ UnitQuantity(fixture.Data(), unit, ANY_CATEGORY) };
  EXPECT_EQ(all.total, 110);
  EXPECT_EQ(all.count, 3);
}

TEST(GroupQuantity, SumsTheReadyUnitsOfTheDivision) {
  Fixture fixture;
  fixture.Field(0, {
                       { TANK_TYPE, 4 }
  });
  fixture.Field(1, {
                       { TANK_TYPE, 6 }
  });
  auto const idle{
    fixture.Field(2, { { TANK_TYPE, 50 } }
     )
  };
  Record<Unit>(fixture.Data(), idle).flags = 0;
  auto const quantity{ GroupQuantity(fixture.Data(), Near(fixture.Data(), fixture.Data().groups[0]), 3) };
  EXPECT_EQ(quantity.total, 10);
  EXPECT_EQ(quantity.count, 2);
}

TEST(SideQuantity, SumsTheReadyDivisionsInto32Bits) {
  Fixture fixture;
  fixture.Field(0, {
                       { INFANTRY_TYPE, 60000 }
  });
  fixture.Field(UNITS_PER_GROUP, {
                                     { INFANTRY_TYPE, 60000 }
  });
  fixture.Data().groups[0].flags |= Bits(GroupFlag::READY);
  fixture.Data().groups[1].flags |= Bits(GroupFlag::READY);
  auto const quantity{ SideQuantity(fixture.Data(), 0, ANY_CATEGORY) };
  EXPECT_EQ(quantity.total, 120000u);
  EXPECT_EQ(quantity.count, 2);
}

TEST(GroupResources, SumTheStoredResourcesAndCapacitiesOfReadyUnits) {
  Fixture    fixture;
  auto const first{
    fixture.Field(0, { { INFANTRY_TYPE, 10 } }
     )
  };
  auto const second{
    fixture.Field(1, { { TANK_TYPE, 2 } }
     )
  };
  Record<Unit>(fixture.Data(), first).resources[0]  = 7;
  Record<Unit>(fixture.Data(), second).resources[0] = 3;
  auto const group{ Near(fixture.Data(), fixture.Data().groups[0]) };
  EXPECT_EQ(GroupStoredResources(fixture.Data(), group).ammunition, 10u);
  EXPECT_EQ(GroupCapacities(fixture.Data(), group).ammunition, 10u * 2 + 2u * 5);
  EXPECT_EQ(GroupResourcePercentages(fixture.Data(), group).ammunition, 10 * 100 / 30);
}

TEST(ObjectiveCapacities, SaturateEachSumAt60000) {
  Fixture fixture;
  auto&   objective{ fixture.Data().objectives[0] };
  objective.items[0]     = { .type = INFANTRY_TYPE, .quantity = 15000, .extra = 0 };
  objective.items[1]     = { .type = INFANTRY_TYPE, .quantity = 10000, .extra = 0 };
  objective.items[2]     = { .type = TANK_TYPE, .quantity = 0, .extra = 0 };
  objective.resources[0] = 25000;
  auto const near{ Near(fixture.Data(), objective) };
  auto const capacities{ ObjectiveCapacities(fixture.Data(), near) };
  EXPECT_EQ(capacities.ammunition, 50000);
  EXPECT_EQ(capacities.fuel, 25000);
  EXPECT_EQ(capacities.food, 60000);
  EXPECT_EQ(ObjectiveResourcePercentages(fixture.Data(), near).ammunition, 50);
}

TEST(ObjectiveQuantity, CountsTheStockedItems) {
  Fixture fixture;
  auto&   objective{ fixture.Data().objectives[0] };
  objective.items[0] = { .type = INFANTRY_TYPE, .quantity = 5, .extra = 0 };
  objective.items[3] = { .type = TANK_TYPE, .quantity = 7, .extra = 0 };
  auto const quantity{ ObjectiveQuantity(fixture.Data(), Near(fixture.Data(), objective), ANY_CATEGORY) };
  EXPECT_EQ(quantity.total, 12);
  EXPECT_EQ(quantity.count, 2);
}
} // namespace opeu::game::detail::resources_test
