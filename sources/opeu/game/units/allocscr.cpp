// The equipment allocation screen: two unit columns, the type list with nation codes, and the per-type remaining and
// reserve amounts.
#include "units/allocscr.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/strings.hpp"
#include "types.hpp"
#include "units/typetext.hpp"

#include "common/game.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {
// The buffer a type's nation code is written to.
constexpr std::uint16_t NATION_BYTES{ 0x1a };

// A unit name centred in its column heading, capped at ten characters.
[[nodiscard]] auto HeadingLength(Game& game, std::uint16_t name) -> std::uint16_t {
  std::uint16_t length = opeu_string_length(game, name);
  return length > 10 ? 10 : length;
}
} // namespace

// Remaining and reserve amounts of each type, one row per type.
void opeu_alloc_amounts_show(Game& game, uint8_t count) {
  auto& data{ Data(game) };
  for (std::uint8_t row{ 0 }; row < count; ++row) {
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x16), static_cast<int16_t>(row * 17 + 0x2e));
    opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(Near(data, data.alloc_amount_format)), static_cast<std::uint16_t>(SideTextColor(data.current_side)),
                                                          static_cast<std::uint16_t>(data.supply_amounts[row]), static_cast<std::uint16_t>(data.supply_others[row]) });
  }
}

// Gives `amount` of the type in `slot` to the stock entry and takes it from the slot and the total.
void opeu_alloc_take(Game& game, uint16_t entry, uint8_t slot, uint16_t amount, uint8_t count) {
  auto& data{ Data(game) };
  auto& item{ Record<Item>(data, entry) };
  item.type                  = data.supply_types[slot];
  item.quantity              = amount;
  data.supply_amounts[slot] -= amount;
  data.supply_total         -= amount;
  opeu_alloc_amounts_show(game, count);
}

// Draws both unit columns (an empty second column takes `third` rows), the names capped at ten characters, and the
// type list; each type's nation code is written to the routine's 22-byte text buffer for printing.
void opeu_alloc_screen(Game& game, uint8_t first, uint8_t second, uint8_t third, uint8_t count) {
  auto&               data{ Data(game) };
  ServiceLocals const nation{ NATION_BYTES };
  auto const          buffer{ nation.At(0) };
  auto const          first_name{ Near(data, data.tally_first_name) };
  auto const          second_name{ Near(data, data.tally_second_name) };

  opeu_backdrop_fill_color(game, static_cast<const UiHost*>(port_uiprim_host()), static_cast<uint8_t>(static_cast<std::uint8_t>(data.current_side ? 1 : 2)));
  opeu_vga_fill_rect(0x2b, 0x110, 0x23, 0x70, 0);
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x158, 0x110, 0x26f, 0x17f, 7);
  auto length{ HeadingLength(game, first_name) };
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x10f, 0x2d, 0x170, 0x3e, 7);
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x15f, 0x2d, 0x170, first * 17 + 0x2d, 7);
  opeu_vga_fill_rect(0x22, 0x2e, 10, 0x10, 3);
  opeu_vga_fill_rect(0x2c, 0x2e, 2, static_cast<std::uint16_t>(static_cast<std::uint16_t>(first) * 17), 3);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x28 - (length >> 1)), static_cast<int16_t>(0x2e));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.alloc_first_format)), static_cast<std::uint16_t>(first_name) });
  if (second == 0) second = third;
  length = HeadingLength(game, second_name);
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x10f, first * 17 + 0x2d, 0x170, first * 17 + 0x3e, 7);
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x15f, first * 17 + 0x2d, 0x170, (first + second) * 17 + 0x2d, 7);
  opeu_vga_fill_rect(0x22, static_cast<std::uint16_t>(static_cast<std::uint16_t>(first) * 17 + 0x2e), 10, 0x10, 5);
  opeu_vga_fill_rect(0x2c, static_cast<std::uint16_t>(static_cast<std::uint16_t>(first) * 17 + 0x2e), 2, static_cast<std::uint16_t>(static_cast<std::uint16_t>(second) * 17 - 1), 5);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x28 - (length >> 1)), static_cast<int16_t>(first * 17 + 0x2e));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.alloc_second_format)), static_cast<std::uint16_t>(second_name) });
  opeu_vga_fill_rect(2, 0x1c, 0x1e, 0xde, 0);
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x10, 0x1c, 0x100, 0xf9, 7);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(3), static_cast<int16_t>(0x1d));
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(Near(data, data.alloc_heading)) });
  for (std::uint8_t row{ 0 }; row < count; ++row) {
    auto const& type{ data.element_types[data.supply_types[row]] };
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(2), static_cast<int16_t>(row * 17 + 0x2e));
    opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(Near(data, data.alloc_row_format)), static_cast<std::uint16_t>(SideTextColor(data.current_side)),
                                                          static_cast<std::uint16_t>(type.name), static_cast<std::uint16_t>(opeu_nation_code(game, Near(data, type), buffer)) });
    opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x10, row * 17 + 0x2d, 0x100, row * 17 + 0x3e, 7);
  }
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x80, 0x1c, 0xa8, 0xf9, 7);
  opeu_alloc_amounts_show(game, count);
}
