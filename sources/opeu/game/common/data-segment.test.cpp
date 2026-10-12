#include "opeu/game/common/data-segment.hpp"

#include "port.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace opeu::game::detail::data_segment {
namespace {
static_assert(sizeof(DataSegment) == 0x10000);
static_assert(UNITS_AT == 0x14fc && GROUPS_AT == 0x0774 && ELEMENTS_AT == 0x07fc && OBJECTIVES_AT == 0x1a7c);
static_assert(sizeof(Unit) * UNITS_PER_SIDE * SIDE_COUNT == sizeof(DataSegment::units));
static_assert(sizeof(Element) * ELEMENTS_PER_SIDE * SIDE_COUNT == sizeof(DataSegment::elements));

// The data image the game starts from: MAIN's data objects and module data copied into a fresh arena.
auto Initial() -> DataSegment const& {
  port_memory_init();
  return Data(port_ds);
}

auto IsElement(std::uint16_t near) -> bool {
  return near >= ELEMENTS_AT && near < UNITS_AT && (near - ELEMENTS_AT) % sizeof(Element) == 0;
}

auto Text(DataSegment const& data, std::uint16_t near) -> std::string_view {
  auto const* bytes{ reinterpret_cast<char const*>(&data) + near };
  return { bytes, std::find(bytes, bytes + 64, '\0') };
}
} // namespace

TEST(DataSegment, CoversTheWholeSegmentAtTheOriginalOffsets) {
  EXPECT_EQ(offsetof(DataSegment, scenario), 0x43u);
  EXPECT_EQ(offsetof(DataSegment, current_side_ptr), 0x64u);
  EXPECT_EQ(offsetof(DataSegment, element_types), 0x3332u);
  EXPECT_EQ(offsetof(DataSegment, stack), 0xebe0u);
  EXPECT_EQ(sizeof(DataSegment::stack), 0x10000u - 0xebe0u);
}

TEST(DataSegment, DecodesTheScenarioHeaderOfTheImage) {
  auto const& data{ Initial() };
  EXPECT_EQ(data.scenario_first, 0x45);
  EXPECT_EQ(data.scenario, 0x80);
  EXPECT_EQ(data.month, 5);
  EXPECT_EQ(data.day, 10);
  EXPECT_EQ(data.map_scale, 2);
  EXPECT_EQ(data.delivery_level, 2);
  EXPECT_EQ(data.side_status[0], 1);
  EXPECT_EQ(data.side_status[1], 1);
  EXPECT_EQ(data.current_side, 0xff);
}

TEST(DataSegment, LinksUnitsToTheirElements) {
  auto const& data{ Initial() };
  auto        linked{ 0 };
  for (auto const& unit : data.units) {
    EXPECT_TRUE(unit.first == 0 || IsElement(unit.first)) << unit.first;
    for (auto near{ unit.first }; near != 0 && linked < 1000; near = Record<Element>(data, near).next) {
      ASSERT_TRUE(IsElement(near)) << near;
      ++linked;
    }
  }
  EXPECT_GT(linked, 0);
}

TEST(DataSegment, NamesEveryElementTypeWithADataSegmentString) {
  auto const& data{ Initial() };
  auto        named{ 0 };
  for (auto const& type : data.element_types) {
    if (type.name == 0) continue;
    EXPECT_FALSE(Text(data, type.name).empty()) << type.name;
    ++named;
  }
  EXPECT_GT(named, 100);
}

TEST(DataSegment, HoldsTheModuleTablesAndFileNames) {
  auto const& data{ Initial() };
  EXPECT_EQ(std::string_view(data.scenario_filename.data()), "SCEDATA");
  auto const days{
    std::array<std::uint8_t, 13>{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
  };
  EXPECT_EQ(data.calendar_days, days);
}

TEST(DataSegment, TurnsNearPointersIntoRecordsAndBack) {
  auto& data{ Data(port_ds) };
  EXPECT_EQ(Near(data, data.units[5]), UNITS_AT + 5 * sizeof(Unit));
  EXPECT_EQ(&Record<Unit>(data, Near(data, data.units[5])), &data.units[5]);
  EXPECT_EQ(Near(data, data.groups[GROUPS_PER_SIDE + 1].flags), GROUPS_AT + 5 * sizeof(Group) + 16);
  EXPECT_EQ(Bits(UnitFlag::READY), 4);
}
} // namespace opeu::game::detail::data_segment
