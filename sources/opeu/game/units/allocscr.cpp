// The equipment allocation screen: two unit columns, the type list with nation codes, and the per-type remaining and
// reserve amounts.
#include "types.hpp"
#include "units/allocscr.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/strings.hpp"
#include "units/typetext.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"

using namespace opeu::game;

namespace {
// The services the screen calls through its event (svc_allocscr).
enum Service : std::uint16_t {
  POSITION = 0, // (x, y)
  PRINT    = 1, // (format, ...)
  FILL     = 2, // (x, y, width, height, colour)
  OUTLINE  = 3, // (x1, y1, x2, y2, colour)
  BACKDROP = 4, // (mode)
};

// The colour of the current side's text.
[[nodiscard]] auto SideColour(DataSegment const& data) -> std::uint16_t {
  return data.current_side ? 5 : 6;
}

// A unit name centred in its column heading, capped at ten characters.
[[nodiscard]] auto HeadingLength(std::uint8_t* ds, std::uint16_t name)
    -> std::uint16_t {
  std::uint16_t length = opeu_string_length(ds, name);
  return length > 10 ? 10 : length;
}
} // namespace

// Remaining and reserve amounts of each type, one row per type.
void opeu_alloc_amounts_show(uint8_t ds[65536], HostWordsEvent event,
                             uint8_t count) {
  auto& data{ Data(ds) };
  for (std::uint8_t row{ 0 }; row < count; ++row) {
    Call(ds, event, POSITION, 0x16, row * 17 + 0x2e);
    Call(ds, event, PRINT, Near(data, data.alloc_amount_format),
         SideColour(data), data.supply_amounts[row], data.supply_others[row]);
  }
}

// Gives `amount` of the type in `slot` to the stock entry and takes it from the slot and the total.
void opeu_alloc_take(uint8_t ds[65536], HostWordsEvent event, uint16_t entry,
                     uint8_t slot, uint16_t amount, uint8_t count) {
  auto& data{ Data(ds) };
  auto& item{ Record<Item>(data, entry) };
  item.type     = data.supply_types[slot];
  item.quantity = amount;
  data.supply_amounts[slot] -= amount;
  data.supply_total -= amount;
  opeu_alloc_amounts_show(ds, event, count);
}

// Draws both unit columns (an empty second column takes `third` rows), the names capped at ten characters, and the
// type list. `buffer` is the caller's 22-byte nation text.
void opeu_alloc_screen(uint8_t ds[65536], HostWordsEvent event, uint8_t first,
                       uint8_t second, uint8_t third, uint8_t count,
                       uint16_t buffer) {
  auto&      data{ Data(ds) };
  auto const first_name{ Near(data, data.tally_first_name) };
  auto const second_name{ Near(data, data.tally_second_name) };

  Call(ds, event, BACKDROP,
       static_cast<std::uint8_t>(data.current_side ? 1 : 2));
  Call(ds, event, FILL, 0x2b, 0x110, 0x23, 0x70, 0);
  Call(ds, event, OUTLINE, 0x158, 0x110, 0x26f, 0x17f, 7);
  auto length{ HeadingLength(ds, first_name) };
  Call(ds, event, OUTLINE, 0x10f, 0x2d, 0x170, 0x3e, 7);
  Call(ds, event, OUTLINE, 0x15f, 0x2d, 0x170, first * 17 + 0x2d, 7);
  Call(ds, event, FILL, 0x22, 0x2e, 10, 0x10, 3);
  Call(ds, event, FILL, 0x2c, 0x2e, 2, static_cast<std::uint16_t>(first) * 17,
       3);
  Call(ds, event, POSITION, 0x28 - (length >> 1), 0x2e);
  Call(ds, event, PRINT, Near(data, data.alloc_first_format), first_name);
  if (second == 0)
    second = third;
  length = HeadingLength(ds, second_name);
  Call(ds, event, OUTLINE, 0x10f, first * 17 + 0x2d, 0x170, first * 17 + 0x3e,
       7);
  Call(ds, event, OUTLINE, 0x15f, first * 17 + 0x2d, 0x170,
       (first + second) * 17 + 0x2d, 7);
  Call(ds, event, FILL, 0x22, static_cast<std::uint16_t>(first) * 17 + 0x2e, 10,
       0x10, 5);
  Call(ds, event, FILL, 0x2c, static_cast<std::uint16_t>(first) * 17 + 0x2e, 2,
       static_cast<std::uint16_t>(second) * 17 - 1, 5);
  Call(ds, event, POSITION, 0x28 - (length >> 1), first * 17 + 0x2e);
  Call(ds, event, PRINT, Near(data, data.alloc_second_format), second_name);
  Call(ds, event, FILL, 2, 0x1c, 0x1e, 0xde, 0);
  Call(ds, event, OUTLINE, 0x10, 0x1c, 0x100, 0xf9, 7);
  Call(ds, event, POSITION, 3, 0x1d);
  Call(ds, event, PRINT, Near(data, data.alloc_heading));
  for (std::uint8_t row{ 0 }; row < count; ++row) {
    auto const& type{ data.element_types[data.supply_types[row]] };
    Call(ds, event, POSITION, 2, row * 17 + 0x2e);
    Call(ds, event, PRINT, Near(data, data.alloc_row_format), SideColour(data),
         type.name, opeu_nation_code(ds, Near(data, type), buffer));
    Call(ds, event, OUTLINE, 0x10, row * 17 + 0x2d, 0x100, row * 17 + 0x3e, 7);
  }
  Call(ds, event, OUTLINE, 0x80, 0x1c, 0xa8, 0xf9, 7);
  opeu_alloc_amounts_show(ds, event, count);
}

// The screen's services: the army text cursor and printer, VGA fill, outlines and the side backdrop.
uint16_t svc_allocscr(uint8_t* ds, uint16_t id, uint16_t* args,
                      uint16_t count) {
  uint16_t r = 0;
  switch (id) {
    case 0:
      opeu_army_position(ds, (TextHost*)port_textout_host(),
                         (int16_t)Arg(args, count, 0),
                         (int16_t)Arg(args, count, 1));
      break;
    case 1: r = lf_cursor_print(ds, args, count); break;
    case 2:
      r = (uint16_t)opeu_vga_fill_rect(Arg(args, count, 0), Arg(args, count, 1),
                                       Arg(args, count, 2), Arg(args, count, 3),
                                       Arg(args, count, 4));
      break;
    case 3:
      opeu_ui_outline(ds, (const UiHost*)port_uiprim_host(),
                      Arg(args, count, 0), Arg(args, count, 1),
                      Arg(args, count, 2), Arg(args, count, 3),
                      Arg(args, count, 4));
      break;
    case 4:
      opeu_backdrop_fill_color(ds, (const UiHost*)port_uiprim_host(),
                               (uint8_t)Arg(args, count, 0));
      break;
    default: port_unhandled("allocscr", id, count);
  }
  return r;
}
