// The division strength line of the current side's army panel: infantry, tanks and artillery of a division.
#include "types.hpp"
#include "core/divstr.hpp"
#include "types/events.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The text services the line calls through its event.
enum Service : std::uint16_t {
  SET_LEFT = 0, // (x)
  POSITION = 1, // (x, y)
  PRINT    = 2, // (format, division, suffix, infantry, tanks, artillery)
};

constexpr std::uint16_t STRENGTH_FORMAT{ offsetof(DataSegment,
                                                  division_strength_format) };
constexpr std::uint16_t RESERVE_MARK{ offsetof(DataSegment,
                                               division_reserve_mark) };
constexpr std::uint16_t NO_MARK{ offsetof(DataSegment, empty_suffix) };
constexpr std::uint16_t LINE_Y{ 0x88 };
} // namespace

// Prints "Div n" (with the reserve mark for a reserve division) and the infantry (in men: categories x1, x10, x5),
// tank and artillery totals of the division's ready or arriving units.
void opeu_show_division_strength(uint8_t ds[65536], HostNotify event,
                                 uint8_t div) {
  auto&         data{ Data(ds) };
  std::uint16_t troops{ 0 };
  std::uint16_t armour{ 0 };
  std::uint16_t artillery{ 0 };
  bool const    in_reserve{ (GroupOf(data, data.current_side, div).flags &
                             Bits(GroupFlag::RESERVE)) != 0 };
  for (auto const& unit : GroupUnits(data, data.current_side, div)) {
    if (!(unit.flags & Bits(UnitFlag::READY)) &&
        !(unit.flags & Bits(UnitFlag::ARRIVING)))
      continue;
    for (auto const& element : Elements(data, unit.first)) {
      auto const count{ element.quantity };
      switch (
          static_cast<Category>(data.element_types[element.type].category)) {
        case Category::INFANTRY:     troops += count; break;
        case Category::INFANTRY_X10: troops += count * 10; break;
        case Category::INFANTRY_X5:  troops += count * 5; break;
        case Category::TANKS:        armour += count; break;
        case Category::ARTILLERY:    artillery += count; break;
      }
    }
  }
  auto const x{ static_cast<std::uint16_t>(div * 20 + 11) };
  Call(ds, event, SET_LEFT, x);
  Call(ds, event, POSITION, x, LINE_Y);
  Call(ds, event, PRINT, STRENGTH_FORMAT, div + 1,
       in_reserve ? RESERVE_MARK : NO_MARK, troops, armour, artillery);
  Call(ds, event, SET_LEFT, 0);
}

void svc_divstr(uint8_t* ds, uint16_t id, const uint16_t* args,
                uint16_t count) {
  switch (id) {
    case SET_LEFT:
      opeu_text_set_left(ds, (TextHost*)port_textout_host(),
                         (int16_t)Arg(args, count, 0));
      break;
    case POSITION:
      opeu_army_position(ds, (TextHost*)port_textout_host(),
                         (int16_t)Arg(args, count, 0),
                         (int16_t)Arg(args, count, 1));
      break;
    case PRINT: b1_cursor_print(ds, args, count); break;
    default:    port_unhandled("divstr", id, count);
  }
}
