// The division panel's regiments: for each ready regiment of a division a battle frame, its heading and, when it can
// act, a slot grid with an icon, name and count for each of its elements.
#include "core/armypanl.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/labels.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t NAME_FORMAT{ offsetof(DataSegment, panel_name_format) };
constexpr std::uint16_t COUNT_FORMAT{ offsetof(DataSegment, panel_count_format) };
constexpr std::uint16_t REGIMENT_FORMAT{ offsetof(DataSegment, panel_regiment_format) };
constexpr std::uint16_t BATTLE_TEXT{ offsetof(DataSegment, panel_battle) };
constexpr std::uint16_t STRING_FORMAT{ offsetof(DataSegment, panel_string_format) };
constexpr std::uint16_t ICONS{ 0x660 }; // eleven 64-byte element icons in the frame segment
constexpr std::uint16_t ICON_BYTES{ 64 };
constexpr std::uint16_t WHITE{ 7 };
constexpr std::uint16_t NAME_BUFFER{ 0x2a }; // the element name buffer the name service writes

} // namespace

// The slot grid: two columns of `count` rows below y.
void opeu_panel_slots(Game& game, int y, uint8_t count) {
  for (int row{ 0 }; row < count; ++row)
    for (int column{ 0 }; column < 2; ++column) {
      auto const x{ 232 * column };
      auto const top{ 17 * row + y };
      game.Machine().Display().Fill(
          {
              { static_cast<uint16_t>(x + 175), static_cast<uint16_t>(top - 1)  },
              { static_cast<uint16_t>(x + 400), static_cast<uint16_t>(top + 15) }
      },
          0);
      opeu_ui_outline(game, x + 175, top - 1, x + 400, top + 16, WHITE);
      game.Machine().Display().Line(
          {
              { static_cast<uint16_t>(x + 207), static_cast<uint16_t>(top - 1)  },
              { static_cast<uint16_t>(x + 207), static_cast<uint16_t>(top + 16) }
      },
          WHITE);
      game.Machine().Display().Line(
          {
              { static_cast<uint16_t>(x + 375), static_cast<uint16_t>(top - 1)  },
              { static_cast<uint16_t>(x + 375), static_cast<uint16_t>(top + 16) }
      },
          WHITE);
    }
}

// The far address (segment in the high word) of the icon of an element's type, chosen by its category and flags.
uint32_t opeu_element_icon(Game const& game, uint16_t element) {
  auto const&  data{ Data(game) };
  auto const&  type{ ElementTypeOf(data, element) };
  std::uint8_t icon{ 0 };
  switch (type.category) {
  case 1:  icon = 1; break;
  case 2:  icon = 2; break;
  case 3:  icon = (type.flags & Bits(TypeFlag::LIGHT)) ? 3 : (type.flags & Bits(TypeFlag::SELF_PROPELLED)) ? 5 : 4; break;
  case 4:  icon = (type.flags & Bits(TypeFlag::HEAVY)) ? 7 : 6; break;
  case 5:  icon = 8; break;
  case 6:  icon = 9; break;
  case 7:  icon = 10; break;
  default: icon = 0; break;
  }
  return static_cast<std::uint32_t>(FRAME_SEGMENT) << 16 | static_cast<std::uint16_t>(ICONS + ICON_BYTES * icon);
}

// Icon, name and count of each element of the unit with a nonzero count, two per row from y.
void opeu_panel_elements(Game& game, uint16_t unit, int y) {
  ServiceLocals const locals{ NAME_BUFFER };
  auto&               data{ Data(game) };
  auto const          buffer{ locals.At(0) };
  int                 shown{ 0 };
  // The walk starts at the unit's first element and stops after the element whose next link is 0.
  for (auto near{ Record<Unit>(data, unit).first };;) {
    auto const& element{ Record<Element>(data, near) };
    if (element.quantity) {
      auto const icon{ opeu_element_icon(game, near) };
      data.gfx_source[0] = static_cast<std::uint16_t>(icon);
      data.gfx_source[1] = static_cast<std::uint16_t>(icon >> 16);
      auto const x{ 29 * (shown % 2) };
      auto const row_y{ 17 * (shown / 2) + y };
      game.Machine().Display().DrawBits({ static_cast<uint16_t>(x + 22), static_cast<uint16_t>(row_y), 4, 16 },
                                        { static_cast<uint16_t>(data.current_side ? 0x305 : 0x306), FarBytes(game, data.gfx_source[1], data.gfx_source[0]) });
      opeu_army_position(game, x + 27, row_y);
      opeu_cursor_print(game, std::array<FormatArgument, 2>{ NAME_FORMAT, opeu_element_name(game, near, buffer) });
      opeu_army_position(game, x + 47, row_y);
      opeu_cursor_print(game, std::array<FormatArgument, 3>{ COUNT_FORMAT, SideTextColor(data.current_side), element.quantity });
      ++shown;
    }
    if (!element.next) break;
    near = element.next;
  }
}

// The battle frame of one regiment at y, joined to the division's line.
void opeu_panel_battle_frame(Game& game, int y) {
  game.Machine().Display().Line(
      {
          { 73, 201 },
          { 84, 201 }
  },
      WHITE);
  game.Machine().Display().Fill(
      {
          { 94,  static_cast<uint16_t>(y)      },
          { 161, static_cast<uint16_t>(y + 34) }
  },
      0);
  opeu_ui_outline(game, 94, y, 161, y + 35, WHITE);
  game.Machine().Display().Line(
      {
          { 84, static_cast<uint16_t>(y + 16) },
          { 94, static_cast<uint16_t>(y + 16) }
  },
      WHITE);
  game.Machine().Display().Line(
      {
          { 84, static_cast<uint16_t>(y + 16) },
          { 84, 200                           }
  },
      WHITE);
}

// The eight regiments of a division from `units` (the view menu's division entry): regiments 1-4 are the battle
// regiments, 5-8 carry their own names.
void opeu_division_regiments(Game& game, uint16_t units, uint16_t person) {
  auto& data{ Data(game) };
  for (int i{ 0 }; i < 8; ++i, units = static_cast<std::uint16_t>(units + sizeof(Unit))) {
    if (!(Record<Unit>(data, units).flags & Bits(UnitFlag::READY))) continue;
    bool const battle{ i < 4 };
    int        y{ battle ? 59 * i + 8 : 42 * i + 68 };
    opeu_panel_battle_frame(game, y);
    y += battle ? 10 : 2;
    opeu_army_position(game, 12, battle ? y - 7 : y - 1);
    opeu_cursor_print(game, std::array<FormatArgument, 4>{ REGIMENT_FORMAT, SideTextColor(data.current_side), (Record<Person>(data, person).assignment & Bits(Assignment::DIVISION_MASK)) + 1, i + 1 });
    if (battle) {
      opeu_army_position(game, 12, y + 9);
      opeu_cursor_print(game, std::array<FormatArgument, 1>{ BATTLE_TEXT });
      y -= 16;
      if (opeu_unit_selection_eligible(game, units)) opeu_panel_slots(game, y, 3);
    } else {
      opeu_army_position(game, 12, y + 15);
      opeu_cursor_print(game, std::array<FormatArgument, 2>{ STRING_FORMAT, data.panel_regiment_names[i - 4] });
      if (opeu_unit_selection_eligible(game, units)) opeu_panel_slots(game, y, 2);
    }
    if (opeu_unit_selection_eligible(game, units)) opeu_panel_elements(game, units, y);
  }
}
