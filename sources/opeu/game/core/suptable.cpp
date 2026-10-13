// The supply-allocation tables and their two list views. Up to twelve type rows (the type, the amount held and a
// second amount, with the sums and averages core/tally keeps) and twelve allocation slots, plus six spare slots.
#include "core/suptable.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/typetext.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/textout.hpp"
#include "io/listui.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

// The services the table views call through their event.
enum class SupplyTableService : std::uint16_t {
  LAYOUT    = 0, // (column, row): the list's column layout
  POSITION  = 1, // (x, y): the text cursor
  PRINT     = 2, // (format, arguments...)
  HIGHLIGHT = 3, // (x, y, width, height, colour)
};
using enum SupplyTableService;

constexpr std::uint8_t  NO_TYPE{ 0xff };   // Item::type of an empty slot
constexpr std::uint16_t LINE_HEIGHT{ 17 }; // pixels per list row
constexpr std::uint8_t  HELD_COLOUR{ 3 };  // the rows of types the side holds
constexpr std::uint8_t  OTHER_COLOUR{ 5 }; // the rows below them
constexpr std::uint8_t  SIDE0_COLOUR{ 6 }; // the type rows of the first side
constexpr std::uint16_t HIGHLIGHT_COLOUR{ 0x107 };
constexpr std::uint16_t NAME_BUFFER{ 32 }; // the type name's buffer, below the nation's in the scratch space

// The near pointer of an equipment type record (an index past the table addresses what follows it).
auto TypeAt(std::uint8_t type) -> std::uint16_t {
  return static_cast<std::uint16_t>(ELEMENT_TYPES_AT + type * sizeof(ElementType));
}

// Works out the list's column layout into a reserve of its own; the table views do not read it.
auto LayOutColumns(Game& game, std::uint8_t column, std::uint8_t row) -> void {
  ServiceLocals const locals{ 4 };
  auto const          out{ locals.At(0) };
  opeu_table_columns(game, column, row, out, static_cast<std::uint16_t>(out + 1), static_cast<std::uint16_t>(out + 2));
}

} // namespace

// The list predicate of the units view: whether the unit on the selected row may be picked.
uint8_t opeu_supply_unit_eligible(Game& game) {
  auto const& data{ Data(game) };
  return opeu_unit_selection_eligible(game, data.supply_units[data.list_row]) ? 1 : 0;
}

// Empties the type rows, their sums and averages, the total and every slot.
void opeu_supply_reset(Game& game) {
  auto& data{ Data(game) };
  for (auto row = 0u; row < data.alloc_slots.size(); ++row) {
    data.supply_morale_sums[row]         = 0;
    data.supply_training_sums[row]       = 0;
    data.supply_experience_sums[row]     = 0;
    data.supply_morale_averages[row]     = 0;
    data.supply_training_averages[row]   = 0;
    data.supply_experience_averages[row] = 0;
    data.alloc_slots[row].type           = NO_TYPE;
    data.supply_total                    = 0;
    data.supply_amounts[row]             = 0;
    data.supply_others[row]              = 0;
    data.alloc_slots[row].quantity       = 0;
    data.alloc_slots[row].extra          = 0;
  }
  for (auto& spare : data.supply_spare) {
    spare.type     = NO_TYPE;
    spare.quantity = 0;
    spare.extra    = 0;
  }
}

uint8_t opeu_supply_true(void) {
  return 1;
}

// Draws the type rows first..last (last below 255); the reserve holds the nation's buffer.
void opeu_supply_draw_types(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  ServiceLocals const locals{ 2 * NAME_BUFFER };
  auto const          scratch{ locals.At(0) };
  auto const&         data{ Data(game) };
  auto const&         window{ ActiveWindow(data) };
  LayOutColumns(game, static_cast<std::uint8_t>(column), static_cast<std::uint8_t>(row));
  if (flags & 3) {
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(window.x + 2), static_cast<int16_t>(window.y + 1));
    opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(offsetof(DataSegment, supply_type_header)) });
  }
  for (std::uint8_t line = first; line <= last; ++line) {
    auto const type{ TypeAt(data.supply_types[line]) };
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(window.x + 2), static_cast<int16_t>((line - data.list_top) * LINE_HEIGHT + window.y + 18));
    auto const amount{ data.supply_amounts[line] };
    auto const other{ data.supply_others[line] };
    auto const nation{ opeu_nation_code(game, type, scratch) };
    opeu_cursor_print(game,
                      std::array<std::uint16_t, 6>{ static_cast<std::uint16_t>(offsetof(DataSegment, supply_type_row)),
                                                    static_cast<std::uint16_t>(data.current_side == 0 ? SIDE0_COLOUR : OTHER_COLOUR), static_cast<std::uint16_t>(Record<ElementType>(data, type).name),
                                                    static_cast<std::uint16_t>(nation), static_cast<std::uint16_t>(amount), static_cast<std::uint16_t>(other) });
  }
}

// Returns the selected slot's amount to its type row and to the free total, and blanks the slot's row.
uint8_t opeu_supply_return_slot(Game& game) {
  auto& data{ Data(game) };
  auto& slot{ data.alloc_slots[data.list_row] };
  for (auto row = 0u; row < data.supply_types.size(); ++row) {
    if (data.supply_types[row] == slot.type) {
      data.supply_amounts[row] += slot.quantity;
      break;
    }
  }
  data.supply_total += slot.quantity;
  slot.type          = NO_TYPE;
  slot.quantity      = 0;
  auto const y{ data.list_row * LINE_HEIGHT + 0x2e };
  opeu_ui_highlight(0x31, y, 0x16, 0x10, HIGHLIGHT_COLOUR);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x2e), static_cast<int16_t>(y));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, supply_returned_slot)),
                                                        static_cast<std::uint16_t>(data.list_row < data.supply_count ? HELD_COLOUR : OTHER_COLOUR) });
  opeu_ui_highlight(0x2e, y, 0x16, 0x10, HIGHLIGHT_COLOUR);
  return 1;
}

// Draws the slot rows first..last; the reserve holds the type name's and, above it, the nation's buffer.
void opeu_supply_draw_slots(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  ServiceLocals const locals{ 2 * NAME_BUFFER };
  auto const          scratch{ locals.At(0) };
  auto const&         data{ Data(game) };
  auto const&         window{ ActiveWindow(data) };
  LayOutColumns(game, static_cast<std::uint8_t>(column), static_cast<std::uint8_t>(row));
  if (flags & 3) {
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(window.x + 2), static_cast<int16_t>(window.y + 1));
    opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(offsetof(DataSegment, supply_slot_header)) });
  }
  for (std::uint16_t line = first; line <= last; ++line) {
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(window.x + 2), static_cast<int16_t>((line - data.list_top) * LINE_HEIGHT + window.y + 18));
    auto const& slot{ data.alloc_slots[line] };
    auto const  colour{ line < data.supply_count ? HELD_COLOUR : OTHER_COLOUR };
    if (slot.quantity) {
      // The name's buffer lies directly below the nation's, so an overlong name runs into it.
      auto const amount{ slot.quantity };
      auto const nation{ opeu_nation_code(game, TypeAt(slot.type), static_cast<std::uint16_t>(scratch + NAME_BUFFER)) };
      auto const name{ opeu_item_type_name(game, Near(data, slot), scratch) };
      opeu_cursor_print(game, std::array<std::uint16_t, 5>{ static_cast<std::uint16_t>(offsetof(DataSegment, supply_slot_row)), static_cast<std::uint16_t>(colour), static_cast<std::uint16_t>(name),
                                                            static_cast<std::uint16_t>(nation), static_cast<std::uint16_t>(amount) });
    } else {
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, supply_empty_slot)), static_cast<std::uint16_t>(colour) });
    }
  }
}

auto port_routine_opeu_supply_draw_slots(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_supply_draw_slots(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2), (uint8_t)Arg(args, count, 3), (uint8_t)Arg(args, count, 4));
  return 0;
}

auto port_routine_opeu_supply_draw_types(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_supply_draw_types(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2), (uint8_t)Arg(args, count, 3), (uint8_t)Arg(args, count, 4));
  return 0;
}

auto port_routine_opeu_supply_return_slot(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_supply_return_slot(game);
}

auto port_routine_opeu_supply_true(Game&, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_supply_true();
}

auto port_routine_opeu_supply_unit_eligible(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_supply_unit_eligible(game);
}
