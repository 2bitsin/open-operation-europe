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
#include "common/memory.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "core/textout.hpp"
#include "map/cellflag.hpp"
#include "map/distance.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/selelem.hpp"

#include "core/dialogs.hpp"
#include "glue.hpp"
#include "io/listdlg.hpp"
#include "io/waitkey.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"

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
// The flags list_dialog runs the transfer's lists with.
constexpr std::uint16_t LIST_LIMIT{ 1 };
constexpr std::uint16_t LIST_FLAGS{ 7 };
// pending_finish's value when the show ends.
constexpr std::uint16_t SHOW_DONE{ 3 };
// The confirmation message buffer the services format and show.
constexpr std::uint16_t MESSAGE_BYTES{ 0x6c };

auto Send(std::uint8_t* ds, HostEvent event, Service service, auto... words) -> std::uint16_t {
  return Call(ds, event, static_cast<std::uint16_t>(service), words...);
}

auto Division(DataSegment& data, unsigned side, unsigned group) -> Group& {
  return data.groups[side * GROUPS_PER_SIDE + group];
}

auto Regiment(DataSegment& data, unsigned side, unsigned group, unsigned index) -> Unit& {
  return data.units[(side * GROUPS_PER_SIDE + group) * 8 + index];
}

// The picked regiment.
auto PickedRegiment(DataSegment& data) -> Unit& {
  return Regiment(data, data.current_side, data.transfer_group, data.transfer_unit);
}

// Points the list window's row predicate at `routine` in the transfer's code segment.
auto SetPredicate(ListWindow& list, Routine routine, std::uint16_t segment) -> void {
  list.select.offset  = routine;
  list.select.segment = segment;
}
} // namespace

// The hours to transfer command from the regiment `from` to the regiment `to` under the general `person`.
uint16_t opeu_transfer_hours(uint8_t ds[65536], uint16_t from, uint16_t to, uint16_t person) {
  auto&               data{ Data(ds) };
  auto const&         source{ Record<Unit>(data, from) };
  auto const&         target{ Record<Unit>(data, to) };
  auto const          side{ opeu_unit_side(to) };
  std::uint16_t const distance{ opeu_grid_distance(source.x, source.y, target.x, target.y) };
  std::uint16_t const supported{ opeu_cell_influence_side(ds, target.x, target.y) == side ? 10u : 0u };
  std::uint16_t const scale{ (data.scenario & 0x7f) == 1 ? 3u : 5u };
  return static_cast<std::uint16_t>(distance / scale * (80 - supported) / (data.sides[side].strength + Record<Person>(data, person).bravery) + 1);
}

// Division list predicate: an active division whose selected regiment may be selected.
uint8_t opeu_transfer_group_enabled(uint8_t ds[65536]) {
  auto&       data{ Data(ds) };
  auto const& division{ Division(data, data.current_side, data.list_row) };
  return (division.flags & Bits(GroupFlag::READY)) && opeu_unit_selection_eligible(ds, opeu_find_group_selected_unit(ds, Near(data, division)));
}

// Regiment list predicate: a ready regiment that may be selected.
uint8_t opeu_transfer_unit_enabled(uint8_t ds[65536]) {
  auto&       data{ Data(ds) };
  auto const& regiment{ Regiment(data, data.current_side, data.transfer_group, data.list_row) };
  return (regiment.flags & Bits(UnitFlag::READY)) && opeu_unit_selection_eligible(ds, Near(data, regiment));
}

// Element list predicate: any element but the current regiment's selected one.
uint8_t opeu_transfer_element_enabled(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
  return opeu_unit_selected_element(ds, data.current_unit_ptr) != data.row_elements[data.list_row];
}

// Lists the picked division's eight regiments.
uint8_t opeu_transfer_units_fill(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
  for (std::uint8_t index{ 0 }; index < 8; ++index) data.row_units[index] = Near(data, Regiment(data, data.current_side, data.transfer_group, index));
  return 8;
}

// Lists the picked regiment's elements; returns their count.
uint8_t opeu_transfer_elements_fill(uint8_t ds[65536]) {
  auto&        data{ Data(ds) };
  std::uint8_t count{ 0 };
  auto const   chain{ Elements(data, PickedRegiment(data)) };
  for (auto it{ chain.begin() }; it != chain.end(); ++it) data.row_elements[count++] = it.Near();
  return count;
}

void opeu_transfer_present(uint8_t ds[65536], HostEvent event) {
  Send(ds, event, PENDING_FINISH, SHOW_DONE);
}

// The transfer dialog; segment: the transfer's code segment.
void opeu_turn_action_3(uint8_t ds[65536], HostEvent event, uint16_t segment) {
  auto&               data{ Data(ds) };
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
      Send(ds, event, CONTROL_MESSAGE, Near(data, data.transfer_which_division), data.context_record2);
      Send(ds, event, CONTROL_PANEL);
      SetPredicate(data.division_dialog, GROUP_ENABLED, segment);
      if (!Send(ds, event, LIST_DIALOG, Near(data, data.division_dialog), LIST_LIMIT, LIST_FLAGS)) return;
      data.transfer_group = data.list_row;
      Send(ds, event, GROUP_SHOW, Near(data, Division(data, data.current_side, data.transfer_group)), data.transfer_group);
      [[fallthrough]];
    case Step::REGIMENT:
      Send(ds, event, CONTROL_MESSAGE, Near(data, data.transfer_which_regiment), data.context_record2);
      Send(ds, event, CONTROL_PANEL);
      data.organize_list.rows = static_cast<std::uint8_t>(opeu_transfer_units_fill(ds) + 1);
      SetPredicate(data.organize_list, UNIT_ENABLED, segment);
      if (!Send(ds, event, LIST_DIALOG, Near(data, data.organize_list), LIST_LIMIT, LIST_FLAGS)) {
        step = Step::DIVISION;
        continue;
      }
      data.transfer_unit = data.list_row;
      Send(ds, event, UNIT_ANNOUNCE, data.transfer_group, Near(data, PickedRegiment(data)));
      [[fallthrough]];
    case Step::ELEMENT: {
      data.choice_dialog.rows = static_cast<std::uint8_t>(opeu_transfer_elements_fill(ds) + 1);
      SetPredicate(data.choice_dialog, ELEMENT_ENABLED, segment);
      Send(ds, event, CONTROL_MESSAGE, Near(data, data.transfer_which_group), data.context_record2);
      Send(ds, event, CONTROL_PANEL);
      if (!Send(ds, event, LIST_DIALOG, Near(data, data.choice_dialog), LIST_LIMIT, LIST_FLAGS)) {
        step = Step::REGIMENT;
        continue;
      }
      auto const hours{ opeu_transfer_hours(ds, data.current_unit_ptr, Near(data, PickedRegiment(data)), data.context_record1) };
      Send(ds, event, REQUEST_FORMAT, buffer, Near(data, data.transfer_confirm), hours, hours == 1 ? Near(data, data.transfer_hour) : Near(data, data.transfer_hours_text));
      if (!Send(ds, event, CONFIRM_MESSAGE, buffer, data.context_record2)) {
        step = Step::ELEMENT;
        continue;
      }
      auto const side{ data.current_side };
      Record<Group>(data, opeu_unit_group(opeu_find_side_selected_unit(ds, side))).flags &= static_cast<std::uint8_t>(~Bits(GroupFlag::SELECTED));
      auto& division{ Division(data, side, data.transfer_group) };
      division.flags                                                                    |= Bits(GroupFlag::SELECTED);
      Record<Unit>(data, opeu_find_group_selected_unit(ds, Near(data, division))).flags &= static_cast<std::uint8_t>(~Bits(UnitFlag::SELECTED));
      auto& regiment{ PickedRegiment(data) };
      regiment.flags                                                                    |= Bits(UnitFlag::SELECTED);
      Record<Element>(data, opeu_unit_selected_element(ds, Near(data, regiment))).flags &= static_cast<std::uint8_t>(~Bits(ElementFlag::SELECTED));
      Record<Element>(data, data.row_elements[data.list_row]).flags                     |= Bits(ElementFlag::SELECTED);
      data.side_delays[side]                                                             = static_cast<std::uint8_t>(hours);
      Send(ds, event, DAILY_SHOW, TRANSFER_PICTURE, data.current_side, PRESENT, segment);
      return;
    }
    }
  }
}

// The routines behind the transfer's calls. unit_announce takes only the regiment.
uint16_t svc_cmdxfer(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case REQUEST_FORMAT:  opeu_request_format(ds, { args, count }); break;
  case CONTROL_MESSAGE: opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case CONFIRM_MESSAGE: r = opeu_confirm_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case CONTROL_PANEL:   opeu_title_panel_begin(ds, (const DialogHost*)port_dialogs_host()); break;
  case LIST_DIALOG:     r = opeu_list_dialog(ds, svc_listdlg, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case GROUP_SHOW:      opeu_group_show(ds, svc_mapunit, Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case UNIT_ANNOUNCE:   opeu_unit_announce(ds, svc_mapunit, Arg(args, count, 1)); break;
  case DAILY_SHOW:
    opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (unsigned int)((uint32_t)Arg(args, count, 3) << 16 | Arg(args, count, 2)));
    break;
  case PENDING_FINISH: opeu_pending_finish(ds, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  default:             port_unhandled("cmdxfer", id, count);
  }
  return r;
}

auto port_routine_opeu_transfer_element_enabled(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_transfer_element_enabled(ds);
}

auto port_routine_opeu_transfer_group_enabled(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_transfer_group_enabled(ds);
}

auto port_routine_opeu_transfer_present(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_transfer_present(ds, svc_cmdxfer);
  return 0;
}

auto port_routine_opeu_transfer_unit_enabled(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_transfer_unit_enabled(ds);
}

// Turn action 3 from the control action table: runs with its own stack and the transfer's code segment.
auto port_routine_opeu_turn_action_3(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_turn_action_3(ds, svc_cmdxfer, TRANSFER_SEGMENT);
  return 0;
}
