#include "opeu/game/common/data-segment.hpp"

#include "port.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
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

TEST(DataSegment, PlacesTheRenamedRecordFields) {
  EXPECT_EQ(offsetof(Side, bombers), 12u);
  EXPECT_EQ(offsetof(Side, pending_bombers), 16u);
  EXPECT_EQ(offsetof(Person, max_health), 23u);
  EXPECT_EQ(offsetof(ElementType, anti_tank), 5u);
  EXPECT_EQ(offsetof(ElementType, anti_personnel), 6u);
  EXPECT_EQ(offsetof(ElementType, defense), 7u);
  EXPECT_EQ(offsetof(ElementType, fuel_economy), 9u);
  EXPECT_EQ(offsetof(ElementType, fuel_capacity), 10u);
  EXPECT_EQ(offsetof(ElementType, gun_range), 11u);
  EXPECT_EQ(offsetof(DataSegment, air_charm), 0xccb8u);
  EXPECT_EQ(offsetof(DataSegment, air_experience), 0xccb9u);
  EXPECT_EQ(offsetof(DataSegment, air_leadership), 0xccbau);
}

TEST(DataSegment, KeepsTheStoredFlagBits) {
  EXPECT_EQ(Bits(ElementFlag::ARRIVING), 0x20);
  EXPECT_EQ(Bits(TypeFlag::AIRBORNE), 0x01);
  EXPECT_EQ(Bits(TypeFlag::SELF_PROPELLED), 0x04);
  EXPECT_EQ(Bits(TypeFlag::HEAVY), 0x08);
  EXPECT_EQ(Bits(TypeFlag::LIGHT), 0x10);
}

TEST(DataSegment, FlagsTheSelfPropelledGunsAmongTheTanksAndTheParatroopsAsInfantry) {
  auto const& data{ Initial() };
  auto const  self_propelled{ std::ranges::count_if(data.element_types, [](auto const& type) { return (type.flags & Bits(TypeFlag::SELF_PROPELLED)) != 0; }) };
  auto const  airborne{ std::ranges::count_if(data.element_types, [](auto const& type) { return (type.flags & Bits(TypeFlag::AIRBORNE)) != 0; }) };
  EXPECT_GT(self_propelled, 20);
  EXPECT_EQ(airborne, 4);
  for (auto const& type : data.element_types) {
    if (type.flags & Bits(TypeFlag::SELF_PROPELLED)) {
      EXPECT_EQ(type.category, Bits(Category::TANKS)) << Text(data, type.name);
    }
    if (type.flags & Bits(TypeFlag::AIRBORNE)) {
      EXPECT_EQ(type.category, Bits(Category::INFANTRY)) << Text(data, type.name);
    }
  }
}

TEST(DataSegment, GivesTheFuelUsersAFuelCapacity) {
  auto const& data{ Initial() };
  for (auto const& type : data.element_types) {
    if (type.name != 0 && type.fuel_economy != 0) {
      EXPECT_NE(type.fuel_capacity, 0) << Text(data, type.name);
    }
  }
}

TEST(DataSegment, LabelsEveryButtonAndEndsEveryMenuWithAnEmptyRecord) {
  auto const& data{ Initial() };
  auto const  menus{ std::to_array({
      std::span<MenuItem const>{ data.scensel_start_menu },
      std::span<MenuItem const>{ data.scensel_players_menu },
      std::span<MenuItem const>{ data.scensel_side_menu },
      std::span<MenuItem const>{ data.scensel_map_menu },
      std::span<MenuItem const>{ data.scensel_battles_menu },
      std::span<MenuItem const>{ data.scensel_battles_menu2 },
      std::span<MenuItem const>{ data.scensel_difficulty_menu },
      std::span<MenuItem const>{ data.scensel_scenario_menu },
      std::span<MenuItem const>{ data.advice_menu },
      std::span<MenuItem const>{ data.staff_menu },
      std::span<MenuItem const>{ data.contact_menu },
      std::span<MenuItem const>{ data.people_menu },
      std::span<MenuItem const>{ data.city_side_menu },
      std::span<MenuItem const>{ data.info_menu },
      std::span<MenuItem const>{ data.arms_menu },
      std::span<MenuItem const>{ data.view_main_menu },
      std::span<MenuItem const>{ data.rq_regiment_kind_menu },
      std::span<MenuItem const>{ data.rq_menu },
      std::span<MenuItem const>{ data.bombing_menu },
      std::span<MenuItem const>{ data.air_mission_menu },
      std::span<MenuItem const>{ data.special_menu },
  }) };
  for (auto const menu : menus) {
    EXPECT_EQ(menu.back().label, 0);
    for (auto const& item : menu.first(menu.size() - 1)) EXPECT_FALSE(Text(data, item.label).empty()) << item.label;
  }
}

TEST(DataSegment, PointsTheListWindowsAtTheirColumnWidths) {
  auto const& data{ Initial() };
  EXPECT_EQ(data.division_dialog.widths, Near(data, data.division_widths));
  EXPECT_EQ(data.order_target_list.widths, Near(data, data.order_target_widths));
  EXPECT_EQ(data.organize_list.widths, Near(data, data.organize_widths));
  EXPECT_EQ(data.regiment_dialog.widths, Near(data, data.regiment_widths));
  EXPECT_EQ(data.choice_dialog.widths, Near(data, data.choice_widths));
  EXPECT_EQ(data.people_dialog.widths, Near(data, data.people_widths));
  EXPECT_EQ(data.weapons_dialog.widths, Near(data, data.weapons_widths));
  EXPECT_EQ(data.city_dialog.widths, Near(data, data.city_widths));
  EXPECT_EQ(data.stock_dialog.widths, Near(data, data.stock_widths));
  EXPECT_EQ(data.save_list.widths, Near(data, data.save_widths));
  EXPECT_EQ(data.play_dialog.widths, Near(data, data.play_widths));
  EXPECT_EQ(data.reinforce_dialog.widths, Near(data, data.reinforce_widths));
  EXPECT_EQ(data.battle_handlers[9].offset, 0x0050);
  EXPECT_EQ(Text(data, data.supply_arrows[1]), "<-");
  EXPECT_EQ(Text(data, data.transfer_arrows[2]), "->");
}

TEST(DataSegment, PointsTheNameTablesAtTheNamedStrings) {
  auto const& data{ Initial() };
  EXPECT_EQ(data.weather_names[0], Near(data, data.weather_clear));
  EXPECT_EQ(data.order_names[Bits(TaskKind::HOLD)], Near(data, data.task_hold));
  EXPECT_EQ(data.order_names[Bits(UnitOrder::LAY_MINES) + 3], Near(data, data.order_lay_mines));
  EXPECT_EQ(data.order_names[Bits(UnitOrder::REST) + 3], Near(data, data.order_rest));
  EXPECT_EQ(data.cursor_code_text[Bits(Terrain::BRIDGE)], Near(data, data.terrain_bridge));
  EXPECT_EQ(data.element_types[2].name, Near(data, data.type_ger_infantry_i));
  EXPECT_EQ(data.element_types[0].name, Near(data, data.type_supply_unit));
  EXPECT_EQ(data.special_mission_names[5], Near(data, data.special_mission_eliminate_cmdr));
  EXPECT_EQ(Text(data, Near(data, data.brief_france_axis_2)).substr(0, 16), "Your mission is ");
  EXPECT_EQ(Text(data, Near(data, data.type_rus_t34_85)), "T34/85");
}

TEST(DataSegment, ReadsTheVisibilityFromTheCoverTableAndNamesTheExplainedGaps) {
  auto const& data{ Initial() };
  EXPECT_EQ(&data.battle_terrain_cover[1], &data.battle_visibility);
  EXPECT_EQ(data.battle_visibility, 1);
  EXPECT_EQ(data.deploy_window.widths, Near(data, data.deploy_widths));
  EXPECT_EQ(data.resistance_names[0], Near(data, data.resistance_partisan));
  EXPECT_TRUE(std::ranges::all_of(data.unused_file_handles, [](auto byte) { return byte == 0xff; }));
  EXPECT_EQ(offsetof(DataSegment, null_area), 0u);
  EXPECT_EQ(offsetof(DataSegment, type_availability), 0x15cu);
  EXPECT_EQ(offsetof(RequestConfig, supply_rate), 11u);
  EXPECT_EQ(offsetof(RequestConfig, reinforcement_rate), 12u);
}
} // namespace opeu::game::detail::data_segment
