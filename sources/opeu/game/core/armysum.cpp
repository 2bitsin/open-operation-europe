// The army summary of the current side's panel: element totals, supply percentages, influence, cities, air strength
// and the headquarters regiment.
#include "core/armysum.hpp"
#include "common/events.hpp"
#include "core/objcount.hpp"
#include "core/selunit.hpp"
#include "map/cellflag.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/aggpct.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t OUT_OF_SUPPLY{ offsetof(DataSegment, army_out) };
constexpr std::uint16_t ROW0{ offsetof(DataSegment, army_row0) };
constexpr std::uint16_t ROW1{ offsetof(DataSegment, army_row1) };
constexpr std::uint16_t ROW2{ offsetof(DataSegment, army_row2) };
constexpr std::uint16_t ROW3{ offsetof(DataSegment, army_row3) };
constexpr std::uint16_t ROW4{ offsetof(DataSegment, army_row4) };
constexpr std::uint16_t ROW5{ offsetof(DataSegment, army_row5) };
constexpr std::uint16_t HQ_FORMAT{ offsetof(DataSegment, army_hq) };
constexpr std::uint8_t  CATEGORIES{ 8 };
constexpr std::uint16_t ROW_X{ 45 };

constexpr auto Low(std::uint32_t value) -> std::uint16_t {
  return static_cast<std::uint16_t>(value);
}
constexpr auto High(std::uint32_t value) -> std::uint16_t {
  return static_cast<std::uint16_t>(value >> 16);
}
constexpr std::uint16_t NAME_BYTES{ 0x50 }; // the headquarters regiment's name
} // namespace

// Shown on a side the player controls (or when neither side is player-controlled). The headquarters regiment's name
// is written into the routine's reserve for the print service.
void opeu_army_summary(Game& game) {
  ServiceLocals const locals{ NAME_BYTES };
  auto&               data{ Data(game) };
  auto const          side{ data.current_side };
  if ((data.side_control[1] || data.side_control[0]) && data.side_control[side] != SIDE_PLAYED) return;
  opeu_army_title(game, data.side_names[side]);
  if (!opeu_unit_support_condition(game, opeu_find_side_selected_unit(game, side))) {
    opeu_ui_text(game, OUT_OF_SUPPLY);
    return;
  }
  std::array<std::uint32_t, CATEGORIES> totals{};
  for (std::uint8_t category{ 0 }; category < CATEGORIES; ++category) totals[category] = SideQuantity(data, side, category).total;
  auto const [ammunition, fuel, food]{ SideUnitResourcePercentages(data, side) };
  std::uint16_t influence{ 0 };
  for (std::uint8_t y{ 0 }; y < MAP_HEIGHT; y += 4)
    for (std::uint8_t x{ 0 }; x < MAP_WIDTH; x += 4)
      if (opeu_cell_influence_side(game, { x, y }) == side) ++influence;
  auto const cities{ opeu_count_active_objectives(game, side) };
  auto const row{ [&](std::uint16_t y) { opeu_army_position(game, ROW_X, y); } };
  row(163);
  opeu_cursor_print(game, std::array<FormatArgument, 6>{ ROW0, Low(totals[3]), High(totals[3]), Low(totals[0]), High(totals[0]), fuel });
  row(181);
  opeu_cursor_print(game, std::array<FormatArgument, 6>{ ROW1, Low(totals[2]), High(totals[2]), Low(totals[5]), High(totals[5]), ammunition });
  row(199);
  opeu_cursor_print(game, std::array<FormatArgument, 6>{ ROW2, Low(totals[1]), High(totals[1]), Low(totals[7]), High(totals[7]), food });
  row(217);
  opeu_cursor_print(game, std::array<FormatArgument, 5>{ ROW3, Low(totals[4]), High(totals[4]), Low(totals[6]), High(totals[6]) });
  auto const& own{ Record<Side>(data, data.current_side_ptr) };
  row(245);
  opeu_cursor_print(game, std::array<FormatArgument, 4>{ ROW4, own.bombers, cities, influence });
  row(263);
  opeu_cursor_print(game, std::array<FormatArgument, 4>{ ROW5, own.planes, own.paratroops, own.strength });
  opeu_army_position(game, 54, 138);
  opeu_cursor_print(game, std::array<FormatArgument, 2>{ HQ_FORMAT, opeu_unit_name(game, opeu_find_side_selected_unit(game, side), locals.At(0)) });
}
