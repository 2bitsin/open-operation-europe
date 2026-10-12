// The supply-allocation tables and their two list views. Up to twelve type rows (the type, the amount held and a
// second amount, with the sums and averages core/tally keeps) and twelve allocation slots, plus six spare slots.
#include "core/suptable.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/typetext.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "glue.hpp"
#include "io/listui.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the table views call through their event.
enum class TableService : std::uint16_t {
  LAYOUT    = 0, // (column, row): the list's column layout
  POSITION  = 1, // (x, y): the text cursor
  PRINT     = 2, // (format, arguments...)
  HIGHLIGHT = 3, // (x, y, width, height, colour)
};
using enum TableService;

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

// The frame of the active list window.
auto Window(DataSegment const& data) -> ListWindow const& {
  return Record<ListWindow>(data, data.list_window);
}
} // namespace

// The list predicate of the units view: whether the unit on the selected row may be picked.
uint8_t opeu_supply_unit_eligible(uint8_t ds[65536]) {
  auto const& data{ Data(ds) };
  return opeu_unit_selection_eligible(ds, data.supply_units[data.list_row]) ? 1 : 0;
}

// Empties the type rows, their sums and averages, the total and every slot.
void opeu_supply_reset(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
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

// Draws the type rows first..last (last below 255); scratch holds the nation's buffer.
void opeu_supply_draw_types(uint8_t ds[65536], HostNotify event, uint16_t scratch, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto const& data{ Data(ds) };
  auto const& window{ Window(data) };
  Call(ds, event, LAYOUT, column, row);
  if (flags & 3) {
    Call(ds, event, POSITION, window.x + 2, window.y + 1);
    Call(ds, event, PRINT, offsetof(DataSegment, supply_type_header));
  }
  for (std::uint8_t line = first; line <= last; ++line) {
    auto const type{ TypeAt(data.supply_types[line]) };
    Call(ds, event, POSITION, window.x + 2, (line - data.list_top) * LINE_HEIGHT + window.y + 18);
    auto const amount{ data.supply_amounts[line] };
    auto const other{ data.supply_others[line] };
    auto const nation{ opeu_nation_code(ds, type, scratch) };
    Call(ds, event, PRINT, offsetof(DataSegment, supply_type_row), data.current_side == 0 ? SIDE0_COLOUR : OTHER_COLOUR, Record<ElementType>(data, type).name, nation, amount, other);
  }
}

// Returns the selected slot's amount to its type row and to the free total, and blanks the slot's row.
uint8_t opeu_supply_return_slot(uint8_t ds[65536], HostNotify event) {
  auto& data{ Data(ds) };
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
  Call(ds, event, HIGHLIGHT, 0x31, y, 0x16, 0x10, HIGHLIGHT_COLOUR);
  Call(ds, event, POSITION, 0x2e, y);
  Call(ds, event, PRINT, offsetof(DataSegment, supply_returned_slot), data.list_row < data.supply_count ? HELD_COLOUR : OTHER_COLOUR);
  Call(ds, event, HIGHLIGHT, 0x2e, y, 0x16, 0x10, HIGHLIGHT_COLOUR);
  return 1;
}

// Draws the slot rows first..last; scratch holds the type name's and, above it, the nation's buffer.
void opeu_supply_draw_slots(uint8_t ds[65536], HostNotify event, uint16_t scratch, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto const& data{ Data(ds) };
  auto const& window{ Window(data) };
  Call(ds, event, LAYOUT, column, row);
  if (flags & 3) {
    Call(ds, event, POSITION, window.x + 2, window.y + 1);
    Call(ds, event, PRINT, offsetof(DataSegment, supply_slot_header));
  }
  for (std::uint16_t line = first; line <= last; ++line) {
    Call(ds, event, POSITION, window.x + 2, (line - data.list_top) * LINE_HEIGHT + window.y + 18);
    auto const& slot{ data.alloc_slots[line] };
    auto const  colour{ line < data.supply_count ? HELD_COLOUR : OTHER_COLOUR };
    if (slot.quantity) {
      // The name's buffer lies directly below the nation's, so an overlong name runs into it.
      auto const amount{ slot.quantity };
      auto const nation{ opeu_nation_code(ds, TypeAt(slot.type), static_cast<std::uint16_t>(scratch + NAME_BUFFER)) };
      auto const name{ opeu_item_type_name(ds, Near(data, slot), scratch) };
      Call(ds, event, PRINT, offsetof(DataSegment, supply_slot_row), colour, name, nation, amount);
    } else {
      Call(ds, event, PRINT, offsetof(DataSegment, supply_empty_slot), colour);
    }
  }
}

void svc_suptable(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (static_cast<TableService>(id)) {
  case LAYOUT: {
    // The layout's three byte outputs go to the DS stack and are not used.
    auto const out{ static_cast<std::uint16_t>(port_stack_enter(4) - 4) };
    opeu_table_columns(ds, svc_listui, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), out, (uint16_t)(out + 1), (uint16_t)(out + 2));
    port_stack_leave(4);
    break;
  }
  case POSITION:  opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case PRINT:     b2_cursor_print(ds, args, count); break;
  case HIGHLIGHT: opeu_ui_highlight(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  default:        port_unhandled("suptable", id, count);
  }
}

namespace {

// Runs a view with the frame its buffers need; scratch lies 0x40 bytes below the frame.
template <typename View> auto WithScratch(View&& view) -> std::uint32_t {
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    view(static_cast<std::uint16_t>(sp - 2 - 0x40));
    return std::uint32_t{ 0 };
  });
}
} // namespace

auto port_routine_opeu_supply_draw_slots(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return WithScratch([&](std::uint16_t scratch) {
    opeu_supply_draw_slots(ds, svc_suptable, scratch, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2), (uint8_t)Arg(args, count, 3),
                           (uint8_t)Arg(args, count, 4));
  });
}

auto port_routine_opeu_supply_draw_types(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return WithScratch([&](std::uint16_t scratch) {
    opeu_supply_draw_types(ds, svc_suptable, scratch, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2), (uint8_t)Arg(args, count, 3),
                           (uint8_t)Arg(args, count, 4));
  });
}

auto port_routine_opeu_supply_return_slot(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_supply_return_slot(ds, svc_suptable);
}

auto port_routine_opeu_supply_true(std::uint8_t*, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_supply_true();
}

auto port_routine_opeu_supply_unit_eligible(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_supply_unit_eligible(ds);
}
