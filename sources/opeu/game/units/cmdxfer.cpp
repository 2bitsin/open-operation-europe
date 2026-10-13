// Turn action 3, transfer of command: the player picks a division of the side, one of its regiments and one of that
// regiment's groups (elements); once the general confirms the estimate, the side's selected division, the division's
// selected regiment and the regiment's selected element move to the picked ones, and the estimate becomes the side's
// delay. The estimate is the current regiment's grid distance to the picked regiment, divided by 3 in scenario 1 (else
// by 5), times 80 (70 when the target's cell is under its own side's influence), divided by the side's strength plus
// the general's bravery, plus one.
#include "units/cmdxfer.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "core/textout.hpp"
#include "map/cellflag.hpp"
#include "map/distance.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/selelem.hpp"

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "io/listdlg.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <iterator>
#include <span>

using namespace opeu::game;

namespace {
// The calls the transfer makes through its event (served by svc_cmdxfer).
enum Service : std::uint16_t {
  REQUEST_FORMAT  = 0, // (destination, format, ...)
  CONTROL_MESSAGE = 1, // (text, person)
  CONFIRM_MESSAGE = 2, // (text, person) -> nonzero when confirmed
  CONTROL_PANEL   = 3, // ()
  LIST_DIALOG     = 4, // (window, limit, flags) -> nonzero when a row was picked
  GROUP_SHOW      = 5, // (division, index)
  UNIT_ANNOUNCE   = 6, // (index, regiment)
  DAILY_SHOW      = 7, // (picture, side, finish offset, finish segment)
  PENDING_FINISH  = 8, // (value)
};

// The link-table addresses of the list predicates and the show's finish routine in the transfer's code segment.
enum Routine : std::uint16_t {
  GROUP_ENABLED   = 0x009a,
  UNIT_ENABLED    = 0x00d4,
  ELEMENT_ENABLED = 0x0110,
  PRESENT         = 0x01ca,
};
constexpr std::uint16_t TRANSFER_PICTURE{ 0x166 };
// The transfer's code segment in the routine link table.
constexpr std::uint16_t TRANSFER_SEGMENT{ 0x1d42 };
// pending_finish's value when the show ends.
constexpr std::uint16_t SHOW_DONE{ 3 };
// The confirmation message buffer the services format and show.
constexpr std::uint16_t MESSAGE_BYTES{ 0x6c };

// The transfer's division's regiments.
auto TransferRegiments(DataSegment& data) -> std::span<Unit, UNITS_PER_GROUP> {
  return GroupUnits(data, data.current_side, data.transfer_group);
}

// The picked regiment.
auto PickedRegiment(DataSegment& data) -> Unit& {
  return TransferRegiments(data)[data.transfer_unit];
}

} // namespace

// The hours to transfer command from the regiment `from` to the regiment `to` under the general `person`.
uint16_t opeu_transfer_hours(Game& game, uint16_t from, uint16_t to, uint16_t person) {
  auto&               data{ Data(game) };
  auto const&         source{ Record<Unit>(data, from) };
  auto const&         target{ Record<Unit>(data, to) };
  auto const          side{ opeu_unit_side(to) };
  std::uint16_t const distance{ opeu_grid_distance(source.x, source.y, target.x, target.y) };
  std::uint16_t const supported{ opeu_cell_influence_side(game, target.x, target.y) == side ? 10u : 0u };
  std::uint16_t const scale{ CurrentScenario(data) == Scenario::AFRICA ? 3u : 5u };
  return static_cast<std::uint16_t>(distance / scale * (80 - supported) / (data.sides[side].strength + Record<Person>(data, person).bravery) + 1);
}

// Division list predicate: an active division whose selected regiment may be selected.
uint8_t opeu_transfer_group_enabled(Game& game) {
  auto&       data{ Data(game) };
  auto const& division{ GroupOf(data, data.current_side, data.list_row) };
  return (division.flags & Bits(GroupFlag::READY)) && opeu_unit_selection_eligible(game, opeu_find_group_selected_unit(game, Near(data, division)));
}

// Regiment list predicate: a ready regiment that may be selected.
uint8_t opeu_transfer_unit_enabled(Game& game) {
  auto&       data{ Data(game) };
  auto const& regiment{ TransferRegiments(data)[data.list_row] };
  return (regiment.flags & Bits(UnitFlag::READY)) && opeu_unit_selection_eligible(game, Near(data, regiment));
}

// Element list predicate: any element but the current regiment's selected one.
uint8_t opeu_transfer_element_enabled(Game& game) {
  auto& data{ Data(game) };
  return opeu_unit_selected_element(game, data.current_unit_ptr) != data.row_elements[data.list_row];
}

// Lists the picked division's eight regiments.
uint8_t opeu_transfer_units_fill(Game& game) {
  auto& data{ Data(game) };
  std::ranges::transform(TransferRegiments(data), std::begin(data.row_units), [&data](Unit const& regiment) { return Near(data, regiment); });
  return 8;
}

// Lists the picked regiment's elements; returns their count.
uint8_t opeu_transfer_elements_fill(Game& game) {
  auto&        data{ Data(game) };
  std::uint8_t count{ 0 };
  auto const   chain{ Elements(data, PickedRegiment(data)) };
  for (auto it{ chain.begin() }; it != chain.end(); ++it) data.row_elements[count++] = it.Near();
  return count;
}

void opeu_transfer_present(Game& game, HostEvent event) {
  Call(game, event, PENDING_FINISH, SHOW_DONE);
}

// The transfer dialog; segment: the transfer's code segment.
void opeu_turn_action_3(Game& game, HostEvent event, uint16_t segment) {
  auto&               data{ Data(game) };
  ServiceLocals const message{ MESSAGE_BYTES };
  auto const          buffer{ message.At(0) };
  enum class Step {
    DIVISION,
    REGIMENT,
    ELEMENT
  } step{ Step::DIVISION };
  for (;;) {
    switch (step) {
    case Step::DIVISION:
      Call(game, event, CONTROL_MESSAGE, Near(data, data.transfer_which_division), data.context_record2);
      Call(game, event, CONTROL_PANEL);
      SetListPredicate(data.division_dialog, GROUP_ENABLED, segment);
      if (!Call(game, event, LIST_DIALOG, Near(data, data.division_dialog), LIST_LIMIT, LIST_FLAGS)) return;
      data.transfer_group = data.list_row;
      Call(game, event, GROUP_SHOW, Near(data, GroupOf(data, data.current_side, data.transfer_group)), data.transfer_group);
      [[fallthrough]];
    case Step::REGIMENT:
      Call(game, event, CONTROL_MESSAGE, Near(data, data.transfer_which_regiment), data.context_record2);
      Call(game, event, CONTROL_PANEL);
      data.organize_list.rows = static_cast<std::uint8_t>(opeu_transfer_units_fill(game) + 1);
      SetListPredicate(data.organize_list, UNIT_ENABLED, segment);
      if (!Call(game, event, LIST_DIALOG, Near(data, data.organize_list), LIST_LIMIT, LIST_FLAGS)) {
        step = Step::DIVISION;
        continue;
      }
      data.transfer_unit = data.list_row;
      Call(game, event, UNIT_ANNOUNCE, data.transfer_group, Near(data, PickedRegiment(data)));
      [[fallthrough]];
    case Step::ELEMENT: {
      data.choice_dialog.rows = static_cast<std::uint8_t>(opeu_transfer_elements_fill(game) + 1);
      SetListPredicate(data.choice_dialog, ELEMENT_ENABLED, segment);
      Call(game, event, CONTROL_MESSAGE, Near(data, data.transfer_which_group), data.context_record2);
      Call(game, event, CONTROL_PANEL);
      if (!Call(game, event, LIST_DIALOG, Near(data, data.choice_dialog), LIST_LIMIT, LIST_FLAGS)) {
        step = Step::REGIMENT;
        continue;
      }
      auto const hours{ opeu_transfer_hours(game, data.current_unit_ptr, Near(data, PickedRegiment(data)), data.context_record1) };
      Call(game, event, REQUEST_FORMAT, buffer, Near(data, data.transfer_confirm), hours, hours == 1 ? Near(data, data.transfer_hour) : Near(data, data.transfer_hours_text));
      if (!Call(game, event, CONFIRM_MESSAGE, buffer, data.context_record2)) {
        step = Step::ELEMENT;
        continue;
      }
      auto const side{ data.current_side };
      Record<Group>(data, opeu_unit_group(opeu_find_side_selected_unit(game, side))).flags &= static_cast<std::uint8_t>(~Bits(GroupFlag::SELECTED));
      auto& division{ GroupOf(data, side, data.transfer_group) };
      division.flags                                                                      |= Bits(GroupFlag::SELECTED);
      Record<Unit>(data, opeu_find_group_selected_unit(game, Near(data, division))).flags &= static_cast<std::uint8_t>(~Bits(UnitFlag::SELECTED));
      auto& regiment{ PickedRegiment(data) };
      regiment.flags                                                                      |= Bits(UnitFlag::SELECTED);
      Record<Element>(data, opeu_unit_selected_element(game, Near(data, regiment))).flags &= static_cast<std::uint8_t>(~Bits(ElementFlag::SELECTED));
      Record<Element>(data, data.row_elements[data.list_row]).flags                       |= Bits(ElementFlag::SELECTED);
      data.side_delays[side]                                                               = static_cast<std::uint8_t>(hours);
      Call(game, event, DAILY_SHOW, TRANSFER_PICTURE, data.current_side, PRESENT, segment);
      return;
    }
    }
  }
}

// The routines behind the transfer's calls. unit_announce takes only the regiment.
uint16_t svc_cmdxfer(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case REQUEST_FORMAT:  opeu_request_format(game, { args, count }); break;
  case CONTROL_MESSAGE: opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case CONFIRM_MESSAGE: r = opeu_confirm_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case CONTROL_PANEL:   opeu_title_panel_begin(game, (const DialogHost*)port_dialogs_host()); break;
  case LIST_DIALOG:     r = opeu_list_dialog(game, svc_listdlg, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case GROUP_SHOW:      opeu_group_show(game, svc_mapunit, Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case UNIT_ANNOUNCE:   opeu_unit_announce(game, svc_mapunit, Arg(args, count, 1)); break;
  case DAILY_SHOW:
    opeu_daily_show(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (unsigned int)((uint32_t)Arg(args, count, 3) << 16 | Arg(args, count, 2)));
    break;
  case PENDING_FINISH: opeu_pending_finish(game, (uint8_t)Arg(args, count, 0)); break;
  default:             port_unhandled("cmdxfer", id, count);
  }
  return r;
}

auto port_routine_opeu_transfer_element_enabled(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_transfer_element_enabled(game);
}

auto port_routine_opeu_transfer_group_enabled(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_transfer_group_enabled(game);
}

auto port_routine_opeu_transfer_present(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_transfer_present(game, svc_cmdxfer);
  return 0;
}

auto port_routine_opeu_transfer_unit_enabled(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_transfer_unit_enabled(game);
}

// Turn action 3 from the control action table: runs with its own stack and the transfer's code segment.
auto port_routine_opeu_turn_action_3(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_turn_action_3(game, svc_cmdxfer, TRANSFER_SEGMENT);
  return 0;
}
