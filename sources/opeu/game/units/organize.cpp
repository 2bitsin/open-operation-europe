// Reorganize command (turn action 6): the player picks a regiment that can be supplied, then a friendly objective
// next to it (to arm from the objective's stock) or a neighbouring regiment of the same class. When the joined
// battalions would not fit, the player may keep only the first regiment's slots. The equipment allocation dialog
// then runs until the player completes or cancels it.
#include "units/organize.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/menu.hpp"
#include "common/view-service.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/chief.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/memory.hpp"
#include "core/recindex.hpp"
#include "core/suptable.hpp"
#include "io/listdlg.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "map/neighbor.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/allocation.hpp"
#include "units/allocdst.hpp"
#include "units/allocpnl.hpp"
#include "units/allocscr.hpp"
#include "units/release.hpp"
#include "units/tally.hpp"
#include "units/typetext.hpp"
#include "units/value.hpp"
#include <array>

using namespace opeu::game;

namespace {

constexpr std::uint16_t DAILY_SHOW_MODE{ 0x2a };
// The list dialog's flags: 7 on the first run, 6 once the screen behind it stands, 11 and 10 likewise in the
// allocation dialog.
constexpr std::uint8_t LIST_FIRST{ 7 };
constexpr std::uint8_t LIST_AGAIN{ 6 };
constexpr std::uint8_t ALLOC_FIRST{ 11 };
constexpr std::uint8_t ALLOC_AGAIN{ 10 };
// Element slots of a regiment: six in an armoured class, four otherwise; and the stock items of an objective.
constexpr std::uint8_t ARMOURED_SLOTS{ 6 };
constexpr std::uint8_t OTHER_SLOTS{ 4 };
constexpr std::uint8_t STOCK_ITEMS{ 6 };

// The prompt the format service writes for the amount to take (the room it had in the command's frame).
constexpr std::size_t PROMPT_BYTES{ 0x64 };

// The command's lists' window states, each saved after its list and restored before it is shown again, and its prompt.
struct Locals {
  ListWindowState          units{};
  ListWindowState          slots{};
  ListWindowState          types{};
  TextBuffer<PROMPT_BYTES> prompt;
};

auto SlotsOf(Game& game, std::uint16_t unit) -> std::uint8_t {
  return opeu_category_0_to_3(opeu_unit_index(unit)) ? ARMOURED_SLOTS : OTHER_SLOTS;
}

// The daily context of the side, drawn as the daily report draws it.
auto Refresh(Game& game) -> void {
  auto& data{ Data(game) };
  data.daily_active = 1;
  opeu_daily_context(game, static_cast<uint8_t>(data.current_side));
  data.daily_active = 0;
}

// Runs the allocation dialog until the player completes or cancels it; a completed allocation delays the side.
auto Allocate(Game& game, Locals& locals, Allocation const& a) -> void {
  auto& data{ Data(game) };
  data.supply_count          = a.slots;
  data.supply_slot_list.rows = static_cast<std::uint8_t>(a.slots + a.other_slots + a.spare_slots + 1);
  data.supply_type_list.rows = static_cast<std::uint8_t>(a.types + 1);
  opeu_alloc_screen(game, a);
  auto const back_to_map{ [&] {
    opeu_screen_reset(game);
    auto const current{ [&]() -> Unit const& { return Record<Unit>(data, data.current_unit_ptr); } };
    opeu_control_position(game, current().x, current().y);
    opeu_arrival_location(game, static_cast<uint8_t>(current().x), static_cast<uint8_t>(current().y), static_cast<uint8_t>(data.current_side));
  } };
  auto flags{ ALLOC_FIRST };
  for (;;) {
    opeu_ops_chief_text(game, Near(data, data.organize_dispatch));
    RestoreListWindow(game, locals.slots);
    if (!opeu_list_dialog(game, data.supply_slot_list, { 1, static_cast<std::uint8_t>(flags) })) {
      if (!data.supply_total && opeu_alloc_groups_nonempty(game, static_cast<uint8_t>(a.slots), static_cast<uint8_t>(a.other_slots)) == 1) {
        if (!opeu_ops_chief_wait(game, Near(data, data.organize_complete))) {
          flags = ALLOC_FIRST;
          continue;
        }
        opeu_alloc_distribute(game, a);
        opeu_alloc_choose_hq(game);
        back_to_map();
        data.side_delays[data.current_side] = 1;
        return;
      }
      if (!opeu_ops_chief_wait(game, Near(data, data.organize_cancel))) {
        flags = ALLOC_FIRST;
        continue;
      }
      back_to_map();
      return;
    }
    locals.slots = SaveListWindow(game);
    std::uint8_t const slot{ data.list_row };
    auto const         item{ Near(data, data.alloc_slots[slot]) };
    std::uint8_t       next{ ALLOC_FIRST };
    for (;;) {
      opeu_ops_chief_text(game, Near(data, data.organize_select));
      RestoreListWindow(game, locals.types);
      if (!opeu_list_dialog(game, data.supply_type_list, { 1, static_cast<std::uint8_t>(flags) })) break;
      locals.types = SaveListWindow(game);
      std::uint8_t const row{ data.list_row };
      auto const&        type{ data.element_types[data.supply_types[row]] };
      opeu_alloc_type_panel(game, static_cast<uint8_t>(row), Near(data, type));
      // A spare slot of the first regiment (drawn from the objective's stock) may take three times the capacity.
      std::uint16_t const capacity{ data.request_capacities[type.category] };
      std::uint16_t const limit{ static_cast<std::uint16_t>(!a.other_slots && a.slots - 1 < slot ? capacity * 3 : capacity) };
      std::uint16_t const most{ limit < data.supply_amounts[row] ? limit : data.supply_amounts[row] };
      opeu_request_format(game, locals.prompt, std::array<FormatArgument, 1>{ Near(data, data.organize_number) });
      auto const amount{ opeu_ops_chief_number(game, locals.prompt, 1, most, 1, 0) };
      if (amount == MENU_CANCELLED) continue;
      opeu_alloc_take(game, { .entry = item, .slot = row, .amount = amount }, a.types);
      next = ALLOC_AGAIN;
      break;
    }
    flags = next;
  }
}
} // namespace

void opeu_turn_action_6(Game& game) {
  auto&      data{ Data(game) };
  Locals     locals;
  auto&      text{ locals.prompt };
  auto const person{ [&] { return data.context_record2; } };
  auto const units{ Near(data, data.row_units) };
  opeu_title_panel_begin(game);
  auto flags{ LIST_FIRST };
  Refresh(game);
  for (;;) {
    RestoreListWindow(game, locals.units);
    data.organize_list.rows = static_cast<std::uint8_t>(opeu_side_supported_units(game, static_cast<uint8_t>(data.current_side), units)) + 1;
    if (data.organize_list.rows == 1) {
      opeu_control_message(game, Near(data, data.organize_none), person());
      return;
    }
    SetListPredicate(data.organize_list, ListSelect::SUPPLY_UNIT_ELIGIBLE);
    opeu_control_message(game, Near(data, data.organize_which), person());
    if (!opeu_list_dialog(game, data.organize_list, { 1, static_cast<std::uint8_t>(flags) })) break;
    locals.units = SaveListWindow(game);
    auto const unit{ data.row_units[data.list_row] };
    opeu_unit_announce(game, flags);
    auto const   elements{ static_cast<std::uint8_t>(opeu_list_length(game, unit)) };
    Allocation   allocation{ .slots = SlotsOf(game, unit), .other_slots = 0, .spare_slots = 0, .types = 0 };
    std::uint8_t other_elements{ 0 };
    opeu_supply_reset(game);
    // A friendly objective on the regiment's cell or next to it that the player accepts.
    bool found{ false };
    for (std::uint8_t direction{ 0 }; direction <= GRID_DIRECTIONS; ++direction) {
      auto const& regiment{ Record<Unit>(data, unit) };
      auto const  next{ StepOnMap({ regiment.x, regiment.y }, direction) };
      if (!next) continue;
      auto const objective{ opeu_find_objective_at(game, next->x, next->y) };
      if (!objective) continue;
      if (static_cast<std::uint8_t>(opeu_objective_side(game, objective)) != data.current_side) continue;
      opeu_request_format(game, text, std::array<FormatArgument, 2>{ Near(data, data.organize_confirm_format), objective });
      if (!opeu_confirm_message(game, text, person())) continue;
      allocation.types       = static_cast<std::uint8_t>(opeu_tally_unit_stock(game, unit, objective));
      allocation.other_slots = 0;
      allocation.spare_slots = static_cast<std::uint8_t>(STOCK_ITEMS - data.tally_reject_count);
      if (allocation.spare_slots == 0) opeu_control_message(game, Near(data, data.organize_no_arms), person());
      found = true;
      break;
    }
    if (!found) {
      // Otherwise a neighbouring regiment of the same class.
      allocation.types = static_cast<std::uint8_t>(opeu_neighbor_units(game, unit, units));
      if (allocation.types == 0) {
        opeu_control_message(game, Near(data, data.organize_no_neighbor), person());
        flags = LIST_AGAIN;
        Refresh(game);
        continue;
      }
      data.organize_list.rows = static_cast<std::uint8_t>(allocation.types + 1);
      SetListPredicate(data.organize_list, ListSelect::SUPPLY_UNIT_ELIGIBLE);
      opeu_control_message(game, Near(data, data.organize_with), person());
      if (!opeu_list_dialog(game, data.organize_list, { 1, static_cast<std::uint8_t>(LIST_FIRST) })) {
        flags = LIST_AGAIN;
        Refresh(game);
        continue;
      }
      auto const other{ data.row_units[data.list_row] };
      opeu_unit_announce(game, flags);
      other_elements         = static_cast<std::uint8_t>(opeu_list_length(game, other));
      allocation.other_slots = SlotsOf(game, other);
      allocation.spare_slots = 0;
      allocation.types       = static_cast<std::uint8_t>(opeu_tally_units(game, unit, other));
    }
    // The battalions must fit the free element slots; otherwise the player may keep only the first regiment's.
    auto const free{ static_cast<std::uint8_t>(opeu_release_side_elements(game, 0, 0)) };
    if (free + other_elements + elements < allocation.slots + allocation.other_slots) {
      if (!opeu_confirm_message(game, Near(data, data.organize_full), person())) {
        flags = LIST_AGAIN;
        Refresh(game);
        continue;
      }
      allocation.slots       = elements;
      allocation.other_slots = other_elements;
    }
    Allocate(game, locals, allocation);
    flags = LIST_AGAIN;
    Refresh(game);
  }
  if (data.side_delays[data.current_side]) opeu_daily_show(game, DAILY_SHOW_MODE, static_cast<uint8_t>(data.current_side), Presenter::ALLOCATION_FINISH);
}
