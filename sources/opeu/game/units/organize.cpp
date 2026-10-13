// Reorganize command (turn action 6): the player picks a regiment that can be supplied, then a friendly objective
// next to it (to arm from the objective's stock) or a neighbouring regiment of the same class. When the joined
// battalions would not fit, the player may keep only the first regiment's slots. The equipment allocation dialog
// then runs until the player completes or cancels it.
#include "units/organize.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "core/chief.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/memory.hpp"
#include "core/recindex.hpp"
#include "core/suptable.hpp"
#include "glue.hpp"
#include "io/listdlg.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "map/neighbor.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/allocdst.hpp"
#include "units/allocpnl.hpp"
#include "units/allocscr.hpp"
#include "units/release.hpp"
#include "units/tally.hpp"
#include "units/typetext.hpp"
#include "units/value.hpp"

using namespace opeu::game;

namespace {
// The services the command calls through its event (svc_organize).
enum class Service : std::uint16_t {
  CONTROL_PANEL,    // ()
  CONTROL_RESET,    // ()
  FORMAT,           // (buffer, format, ...)
  CONTROL_MESSAGE,  // (text, person)
  CONTROL_CONFIRM,  // (text, person) -> nonzero for yes
  LIST_DIALOG,      // (list, mode, flags) -> nonzero when a row was picked
  GROUP_NOTICE,     // (flags, unit)
  COPY_BYTES,       // (source, destination, size)
  DAILY_CONTEXT,    // (side)
  DAILY_SHOW,       // (mode, side, offset, segment)
  UNIT_INDEX,       // (unit) -> its index among all regiments
  UNIT_CLASS,       // (index) -> nonzero for an armoured class
  GRID_STEP,        // (x address, y address, direction) -> nonzero when the step stays on the map
  FIND_OBJECTIVE,   // (x, y) -> the objective on the cell, 0 for none
  OBJECTIVE_SIDE,   // (objective) -> side
  RELEASE_ELEMENTS, // (0, 0) -> the free element slots
  CONTROL_POSITION, // (x, y)
  ARRIVAL_LOCATION, // (x, y, side)
  LIST_LENGTH,      // (unit) -> its elements
  SUPPORTED_UNITS,  // (side, list) -> the regiments that can be supplied
  NEIGHBOR_UNITS,   // (unit, list) -> the neighbouring regiments of its class
  CHIEF_TEXT,       // (text)
  CHIEF_WAIT,       // (text) -> nonzero for yes
  CHIEF_NUMBER,     // (text, low, high, step, start) -> the number, 0xffff when cancelled
  SUPPLY_RESET,     // ()
  TALLY_STOCK,      // (unit, objective) -> the types in the allocation
  TALLY_UNITS,      // (unit, other) -> the types in the allocation
  ALLOC_SCREEN,     // (slots, other slots, spare slots, types)
  ALLOC_TAKE,       // (slot, type row, amount, types)
  ALLOC_NONEMPTY,   // (slots, other slots) -> 1 when both units are given something
  ALLOC_TYPE_PANEL, // (type row, type)
  ALLOC_DISTRIBUTE, // (slots, other slots, spare slots, types)
  ALLOC_CHOOSE_HQ,  // ()
};

// The command's code segment and the link-table addresses of its routines that the dialogs call back.
constexpr std::uint16_t ORGANIZE_SEGMENT{ 0x20b3 };
constexpr std::uint16_t UNIT_ENABLED{ 0x0d40 }; // the list predicate: a regiment that can be supplied
constexpr std::uint16_t DAILY_REDRAW{ 0x1f46 }; // the screen the daily report redraws
constexpr std::uint16_t DAILY_SHOW_MODE{ 0x2a };
// The list dialog's flags: 7 on the first run, 6 once the screen behind it stands, 11 and 10 likewise in the
// allocation dialog.
constexpr std::uint8_t LIST_FIRST{ 7 };
constexpr std::uint8_t LIST_AGAIN{ 6 };
constexpr std::uint8_t ALLOC_FIRST{ 11 };
constexpr std::uint8_t ALLOC_AGAIN{ 10 };
// The list dialog's state from list_window on, saved around each nested list.
constexpr std::uint16_t LIST_STATE_SIZE{ 7 };
// Element slots of a regiment: six in an armoured class, four otherwise; and the stock items of an objective.
constexpr std::uint8_t ARMOURED_SLOTS{ 6 };
constexpr std::uint8_t OTHER_SLOTS{ 4 };
constexpr std::uint8_t STOCK_ITEMS{ 6 };

// The command's locals whose near addresses reach services, at their places below the top of its reserve.
enum Locals : std::uint16_t {
  STEP_X      = 0x1a, // the cell grid_step moves
  STEP_Y      = 0x1e,
  UNIT_LIST   = 0x2e, // the saved list states
  SLOT_LIST   = 0x9e,
  TYPE_LIST   = 0xa8,
  PROMPT_TEXT = 0x92,
};
constexpr std::uint16_t LOCALS_BYTES{ TYPE_LIST };

// The near address of one of the command's locals in its reserve.
auto LocalAt(ServiceLocals const& locals, Locals local) -> std::uint16_t {
  return locals.At(static_cast<std::uint16_t>(LOCALS_BYTES - local));
}

auto SlotsOf(std::uint8_t* ds, HostEvent event, std::uint16_t unit) -> std::uint8_t {
  return Call(ds, event, Service::UNIT_CLASS, static_cast<std::uint8_t>(Call(ds, event, Service::UNIT_INDEX, unit))) ? ARMOURED_SLOTS : OTHER_SLOTS;
}

auto SetEnabled(ListWindow& list, std::uint16_t segment) -> void {
  list.select.offset  = UNIT_ENABLED;
  list.select.segment = segment;
}

// The daily context of the side, drawn as the daily report draws it.
auto Refresh(std::uint8_t* ds, HostEvent event) -> void {
  auto& data{ Data(ds) };
  data.daily_active = 1;
  Call(ds, event, Service::DAILY_CONTEXT, data.current_side);
  data.daily_active = 0;
}

// The allocation the player edits.
struct Allocation {
  std::uint8_t slots;       // the first regiment's slots (t)
  std::uint8_t other_slots; // the second regiment's slots, 0 with an objective (U)
  std::uint8_t spare_slots; // the objective's stock items the first regiment may draw (y)
  std::uint8_t types;       // the types in the allocation (r)
};

// Runs the allocation dialog until the player completes or cancels it; a completed allocation delays the side.
auto Allocate(std::uint8_t* ds, HostEvent event, ServiceLocals const& locals, Allocation const& a) -> void {
  auto&      data{ Data(ds) };
  auto const state{ Near(data, data.list_window) };
  auto const text{ LocalAt(locals, PROMPT_TEXT) };
  data.supply_count          = a.slots;
  data.supply_slot_list.rows = static_cast<std::uint8_t>(a.slots + a.other_slots + a.spare_slots + 1);
  data.supply_type_list.rows = static_cast<std::uint8_t>(a.types + 1);
  Call(ds, event, Service::ALLOC_SCREEN, data.supply_count, a.other_slots, a.spare_slots, a.types);
  auto const back_to_map{ [&] {
    Call(ds, event, Service::CONTROL_RESET);
    auto const current{ [&]() -> Unit const& { return Record<Unit>(data, data.current_unit_ptr); } };
    Call(ds, event, Service::CONTROL_POSITION, current().x, current().y);
    Call(ds, event, Service::ARRIVAL_LOCATION, current().x, current().y, data.current_side);
  } };
  auto flags{ ALLOC_FIRST };
  for (;;) {
    Call(ds, event, Service::CHIEF_TEXT, Near(data, data.organize_dispatch));
    Call(ds, event, Service::COPY_BYTES, LocalAt(locals, SLOT_LIST), state, LIST_STATE_SIZE);
    if (!Call(ds, event, Service::LIST_DIALOG, Near(data, data.supply_slot_list), 1, flags)) {
      if (!data.supply_total && Call(ds, event, Service::ALLOC_NONEMPTY, a.slots, a.other_slots) == 1) {
        if (!Call(ds, event, Service::CHIEF_WAIT, Near(data, data.organize_complete))) {
          flags = ALLOC_FIRST;
          continue;
        }
        Call(ds, event, Service::ALLOC_DISTRIBUTE, a.slots, a.other_slots, a.spare_slots, a.types);
        Call(ds, event, Service::ALLOC_CHOOSE_HQ);
        back_to_map();
        data.side_delays[data.current_side] = 1;
        return;
      }
      if (!Call(ds, event, Service::CHIEF_WAIT, Near(data, data.organize_cancel))) {
        flags = ALLOC_FIRST;
        continue;
      }
      back_to_map();
      return;
    }
    Call(ds, event, Service::COPY_BYTES, state, LocalAt(locals, SLOT_LIST), LIST_STATE_SIZE);
    std::uint8_t const slot{ data.list_row };
    auto const         item{ Near(data, data.alloc_slots[slot]) };
    std::uint8_t       next{ ALLOC_FIRST };
    for (;;) {
      Call(ds, event, Service::CHIEF_TEXT, Near(data, data.organize_select));
      Call(ds, event, Service::COPY_BYTES, LocalAt(locals, TYPE_LIST), state, LIST_STATE_SIZE);
      if (!Call(ds, event, Service::LIST_DIALOG, Near(data, data.supply_type_list), 1, flags)) break;
      Call(ds, event, Service::COPY_BYTES, state, LocalAt(locals, TYPE_LIST), LIST_STATE_SIZE);
      std::uint8_t const row{ data.list_row };
      auto const&        type{ data.element_types[data.supply_types[row]] };
      Call(ds, event, Service::ALLOC_TYPE_PANEL, row, Near(data, type));
      // A spare slot of the first regiment (drawn from the objective's stock) may take three times the capacity.
      std::uint16_t const capacity{ data.request_capacities[type.category] };
      std::uint16_t const limit{ static_cast<std::uint16_t>(!a.other_slots && a.slots - 1 < slot ? capacity * 3 : capacity) };
      std::uint16_t const most{ limit < data.supply_amounts[row] ? limit : data.supply_amounts[row] };
      Call(ds, event, Service::FORMAT, text, Near(data, data.organize_number));
      auto const amount{ Call(ds, event, Service::CHIEF_NUMBER, text, 1, most, 1, 0) };
      if (amount == MENU_CANCELLED) continue;
      Call(ds, event, Service::ALLOC_TAKE, item, row, amount, a.types);
      next = ALLOC_AGAIN;
      break;
    }
    flags = next;
  }
}
} // namespace

// segment: the command's code segment.
void opeu_turn_action_6(uint8_t ds[65536], HostEvent event, uint16_t segment) {
  auto&               data{ Data(ds) };
  ServiceLocals const locals{ LOCALS_BYTES };
  auto const          state{ Near(data, data.list_window) };
  auto const          text{ LocalAt(locals, PROMPT_TEXT) };
  auto const          person{ [&] { return data.context_record2; } };
  auto const          units{ Near(data, data.row_units) };
  Call(ds, event, Service::CONTROL_PANEL);
  auto flags{ LIST_FIRST };
  Refresh(ds, event);
  for (;;) {
    Call(ds, event, Service::COPY_BYTES, LocalAt(locals, UNIT_LIST), state, LIST_STATE_SIZE);
    data.organize_list.rows = static_cast<std::uint8_t>(Call(ds, event, Service::SUPPORTED_UNITS, data.current_side, units)) + 1;
    if (data.organize_list.rows == 1) {
      Call(ds, event, Service::CONTROL_MESSAGE, Near(data, data.organize_none), person());
      return;
    }
    SetEnabled(data.organize_list, segment);
    Call(ds, event, Service::CONTROL_MESSAGE, Near(data, data.organize_which), person());
    if (!Call(ds, event, Service::LIST_DIALOG, Near(data, data.organize_list), 1, flags)) break;
    Call(ds, event, Service::COPY_BYTES, state, LocalAt(locals, UNIT_LIST), LIST_STATE_SIZE);
    auto const unit{ data.row_units[data.list_row] };
    Call(ds, event, Service::GROUP_NOTICE, flags, unit);
    auto const   elements{ static_cast<std::uint8_t>(Call(ds, event, Service::LIST_LENGTH, unit)) };
    Allocation   allocation{ .slots = SlotsOf(ds, event, unit), .other_slots = 0, .spare_slots = 0, .types = 0 };
    std::uint8_t other_elements{ 0 };
    Call(ds, event, Service::SUPPLY_RESET);
    // A friendly objective on the regiment's cell or next to it that the player accepts.
    bool       found{ false };
    auto const step_x{ LocalAt(locals, STEP_X) };
    auto const step_y{ LocalAt(locals, STEP_Y) };
    for (std::uint8_t direction{ 0 }; direction <= GRID_DIRECTIONS; ++direction) {
      auto const& regiment{ Record<Unit>(data, unit) };
      Byte(ds, step_x) = regiment.x;
      Byte(ds, step_y) = regiment.y;
      if (!Call(ds, event, Service::GRID_STEP, step_x, step_y, direction)) continue;
      auto const objective{ Call(ds, event, Service::FIND_OBJECTIVE, Byte(ds, step_x), Byte(ds, step_y)) };
      if (!objective) continue;
      if (static_cast<std::uint8_t>(Call(ds, event, Service::OBJECTIVE_SIDE, objective)) != data.current_side) continue;
      Call(ds, event, Service::FORMAT, text, Near(data, data.organize_confirm_format), objective);
      if (!Call(ds, event, Service::CONTROL_CONFIRM, text, person())) continue;
      allocation.types       = static_cast<std::uint8_t>(Call(ds, event, Service::TALLY_STOCK, unit, objective));
      allocation.other_slots = 0;
      allocation.spare_slots = static_cast<std::uint8_t>(STOCK_ITEMS - data.tally_reject_count);
      if (allocation.spare_slots == 0) Call(ds, event, Service::CONTROL_MESSAGE, Near(data, data.organize_no_arms), person());
      found = true;
      break;
    }
    if (!found) {
      // Otherwise a neighbouring regiment of the same class.
      allocation.types = static_cast<std::uint8_t>(Call(ds, event, Service::NEIGHBOR_UNITS, unit, units));
      if (allocation.types == 0) {
        Call(ds, event, Service::CONTROL_MESSAGE, Near(data, data.organize_no_neighbor), person());
        flags = LIST_AGAIN;
        Refresh(ds, event);
        continue;
      }
      data.organize_list.rows = static_cast<std::uint8_t>(allocation.types + 1);
      SetEnabled(data.organize_list, segment);
      Call(ds, event, Service::CONTROL_MESSAGE, Near(data, data.organize_with), person());
      if (!Call(ds, event, Service::LIST_DIALOG, Near(data, data.organize_list), 1, LIST_FIRST)) {
        flags = LIST_AGAIN;
        Refresh(ds, event);
        continue;
      }
      auto const other{ data.row_units[data.list_row] };
      Call(ds, event, Service::GROUP_NOTICE, flags, other);
      other_elements         = static_cast<std::uint8_t>(Call(ds, event, Service::LIST_LENGTH, other));
      allocation.other_slots = SlotsOf(ds, event, other);
      allocation.spare_slots = 0;
      allocation.types       = static_cast<std::uint8_t>(Call(ds, event, Service::TALLY_UNITS, unit, other));
    }
    // The battalions must fit the free element slots; otherwise the player may keep only the first regiment's.
    auto const free{ static_cast<std::uint8_t>(Call(ds, event, Service::RELEASE_ELEMENTS, 0, 0)) };
    if (free + other_elements + elements < allocation.slots + allocation.other_slots) {
      if (!Call(ds, event, Service::CONTROL_CONFIRM, Near(data, data.organize_full), person())) {
        flags = LIST_AGAIN;
        Refresh(ds, event);
        continue;
      }
      allocation.slots       = elements;
      allocation.other_slots = other_elements;
    }
    Allocate(ds, event, locals, allocation);
    flags = LIST_AGAIN;
    Refresh(ds, event);
  }
  if (data.side_delays[data.current_side]) Call(ds, event, Service::DAILY_SHOW, DAILY_SHOW_MODE, data.current_side, DAILY_REDRAW, segment);
}

// The command's services.
uint16_t svc_organize(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (static_cast<Service>(id)) {
  case Service::CONTROL_PANEL:   opeu_title_panel_begin(ds, (const DialogHost*)port_dialogs_host()); break;
  case Service::CONTROL_RESET:   opeu_screen_reset(ds, (const DialogHost*)port_dialogs_host()); break;
  case Service::FORMAT:          opeu_request_format(ds, { args, count }); break;
  case Service::CONTROL_MESSAGE: opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::CONTROL_CONFIRM: r = opeu_confirm_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::LIST_DIALOG:     r = opeu_list_dialog(ds, svc_listdlg, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case Service::GROUP_NOTICE:    // The game passes (flags, unit) to the announcement, whose one parameter receives the flags word.
    opeu_unit_announce(ds, svc_mapunit, Arg(args, count, 0));
    break;
  case Service::COPY_BYTES:    opeu_copy_bytes(ds, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case Service::DAILY_CONTEXT: opeu_daily_context(ds, svc_context, (uint8_t)Arg(args, count, 0)); break;
  case Service::DAILY_SHOW:
    opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint32_t)Arg(args, count, 3) << 16 | Arg(args, count, 2));
    break;
  case Service::UNIT_INDEX:       r = opeu_unit_index(Arg(args, count, 0)); break;
  case Service::UNIT_CLASS:       r = opeu_category_0_to_3((uint8_t)Arg(args, count, 0)); break;
  case Service::GRID_STEP:        r = opeu_grid_step(PORT_DS(Arg(args, count, 0)), PORT_DS(Arg(args, count, 1)), (uint8_t)Arg(args, count, 2)); break;
  case Service::FIND_OBJECTIVE:   r = opeu_find_objective_at(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case Service::OBJECTIVE_SIDE:   r = opeu_objective_side(ds, Arg(args, count, 0)); break;
  case Service::RELEASE_ELEMENTS: r = opeu_release_side_elements(ds, Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case Service::CONTROL_POSITION: opeu_control_position(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::ARRIVAL_LOCATION: opeu_arrival_location(ds, svc_mappos, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case Service::LIST_LENGTH:      r = opeu_list_length(ds, Arg(args, count, 0)); break;
  case Service::SUPPORTED_UNITS:  r = opeu_side_supported_units(ds, (uint8_t)Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::NEIGHBOR_UNITS:   r = opeu_neighbor_units(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::CHIEF_TEXT:       opeu_ops_chief_text(ds, svc_chief, Arg(args, count, 0)); break;
  case Service::CHIEF_WAIT:       r = opeu_ops_chief_wait(ds, svc_chief, Arg(args, count, 0)); break;
  case Service::CHIEF_NUMBER:     r = opeu_ops_chief_number(ds, svc_chief, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case Service::SUPPLY_RESET:     opeu_supply_reset(ds); break;
  case Service::TALLY_STOCK:      r = opeu_tally_unit_stock(ds, svc_tally, Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::TALLY_UNITS:      r = opeu_tally_units(ds, svc_tally, Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::ALLOC_SCREEN:     opeu_alloc_screen(ds, svc_allocscr, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2), (uint8_t)Arg(args, count, 3)); break;
  case Service::ALLOC_TAKE:       opeu_alloc_take(ds, svc_allocscr, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), Arg(args, count, 2), (uint8_t)Arg(args, count, 3)); break;
  case Service::ALLOC_NONEMPTY:   r = opeu_alloc_groups_nonempty(ds, svc_allocpnl, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case Service::ALLOC_TYPE_PANEL: opeu_alloc_type_panel(ds, svc_allocpnl, (uint8_t)Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::ALLOC_DISTRIBUTE:
    opeu_alloc_distribute(ds, svc_allocdst, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2), (uint8_t)Arg(args, count, 3));
    break;
  case Service::ALLOC_CHOOSE_HQ: opeu_alloc_choose_hq(ds, svc_allocdst, ORGANIZE_SEGMENT); break;
  default:                       port_unhandled("organize", id, count);
  }
  return r;
}

auto port_routine_opeu_turn_action_6(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_turn_action_6(ds, svc_organize, ORGANIZE_SEGMENT);
  return 0;
}
