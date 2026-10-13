// The division strength line of the current side's army panel: infantry, tanks and artillery of a division.
#include "core/divstr.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t STRENGTH_FORMAT{ offsetof(DataSegment, division_strength_format) };
constexpr std::uint16_t RESERVE_MARK{ offsetof(DataSegment, division_reserve_mark) };
constexpr std::uint16_t NO_MARK{ offsetof(DataSegment, empty_suffix) };
constexpr std::uint16_t LINE_Y{ 0x88 };
} // namespace

// Prints "Div n" (with the reserve mark for a reserve division) and the infantry (in men: categories x1, x10, x5),
// tank and artillery totals of the division's ready or arriving units.
void opeu_show_division_strength(Game& game, uint8_t div) {
  auto&         data{ Data(game) };
  std::uint16_t troops{ 0 };
  std::uint16_t armour{ 0 };
  std::uint16_t artillery{ 0 };
  bool const    in_reserve{ (GroupOf(data, data.current_side, div).flags & Bits(GroupFlag::RESERVE)) != 0 };
  for (auto const& unit : GroupUnits(data, data.current_side, div)) {
    if (!(unit.flags & Bits(UnitFlag::READY)) && !(unit.flags & Bits(UnitFlag::ARRIVING))) continue;
    for (auto const& element : Elements(data, unit.first)) {
      auto const count{ element.quantity };
      switch (static_cast<Category>(data.element_types[element.type].category)) {
      case Category::INFANTRY:     troops += count; break;
      case Category::INFANTRY_X10: troops += count * 10; break;
      case Category::INFANTRY_X5:  troops += count * 5; break;
      case Category::TANKS:        armour += count; break;
      case Category::ARTILLERY:    artillery += count; break;
      default:                     break; // engineers, repair and supply units are not counted
      }
    }
  }
  auto const x{ static_cast<std::uint16_t>(div * 20 + 11) };
  opeu_text_set_left(game, static_cast<TextHost*>(port_textout_host()), x);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), x, LINE_Y);
  opeu_cursor_print(game, std::array<std::uint16_t, 6>{ STRENGTH_FORMAT, static_cast<std::uint16_t>(div + 1), (in_reserve ? RESERVE_MARK : NO_MARK), troops, armour, artillery });
  opeu_text_set_left(game, static_cast<TextHost*>(port_textout_host()), 0);
}
