// The personnel orders menu (turn action 4): putting a division commander in charge, assigning a free general to a
// division or a staff post, and sending a general on leave.
#include "core/persmenu.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"
#include "core/counter.hpp"
#include "core/people.hpp"
#include "core/personfl.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include <array>
#include <span>
#include <utility>

#include "core/dialogs.hpp"
#include "core/selunit.hpp"
#include "glue.hpp"
#include "io/listdlg.hpp"
#include "io/waitkey.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// The services the personnel routines call through their event.
enum class PersonnelService : uint16_t {
  FORMAT          = 0, // (buffer, format, words...)
  MESSAGE         = 1, // (text, person): a general's message
  PANEL           = 2, // redraw the control panel
  DAILY_SHOW      = 3, // (id, side, presenter offset, presenter segment)
  PENDING_FINISH  = 4, // (value)
  LIST            = 5, // (list, limit, flags) -> 0 when cancelled
  ASK             = 6, // (text, last, person, menu) -> the choice, MENU_CANCELLED when cancelled
  GROUP_SELECTED  = 7, // (group) -> the division's selected regiment
  UNIT_SELECTABLE = 8, // (unit)
  RUN             = 9, // (segment, offset): a far action
};

// Offsets of the personnel routines in their code segment, which the list and show callbacks are given as.
constexpr uint16_t FINISH_OFFSET{ 0x0004 };
constexpr uint16_t ALWAYS_OFFSET{ 0x000c };
constexpr uint16_t TOGGLE_OFFSET{ 0x003c };
// The code segment the personnel routines run in.
constexpr uint16_t PERSONNEL_SEGMENT{ 0x1d82 };

constexpr uint16_t ASSIGN_SHOW{ 0x17b };
constexpr uint8_t  ASSIGN_FINISH{ 3 };
constexpr uint8_t  LIST_LIMIT{ 1 };
constexpr uint8_t  LIST_FLAGS{ 7 };
constexpr uint8_t  POSTS{ 6 };       // four divisions, operations and intelligence
constexpr uint8_t  REST_LAST{ 10 };  // the most days of leave less one
constexpr uint8_t  ORDERS_LAST{ 3 }; // reassign, appoint, rest
constexpr uint8_t  BUSY_ROLES{ Bits(PersonRole::ROLE_MASK) | Bits(PersonRole::DEPUTY) };

// The texts the format service writes for the assignment menu (one 30-byte line per post) and its prompt, in the
// routine's reserve. The menu keeps the labels' near addresses after the routine returns; a later list draw reads
// what is left there (#13 gives the labels a home of their own).
constexpr uint16_t POST_TEXT_BYTES{ 0x1e };
constexpr uint16_t ASSIGN_PROMPT_BYTES{ 0x6c };
// The leave prompt, which the format service writes.
constexpr uint16_t REST_PROMPT_BYTES{ 0x6a };

// Shows the list window; 0 when it is cancelled.
auto ShowList(uint8_t* ds, HostEvent event, ListWindow const& list) -> uint16_t {
  return Call(ds, event, PersonnelService::LIST, Near(Data(ds), list), LIST_LIMIT, LIST_FLAGS);
}

// Lists the side's free generals and sizes the generals' list to them (one row more for the title).
auto FillGenerals(uint8_t* ds, HostEvent event, uint16_t segment) -> uint8_t {
  auto& list{ Data(ds).people_dialog };
  list.rows = opeu_idle_commanders_fill(ds, event, segment) + 1;
  return list.rows;
}

} // namespace

// The picture callback of the assignment's confirmation.
void opeu_personnel_finish(uint8_t ds[65536], HostEvent event, uint16_t) {
  Call(ds, event, PersonnelService::PENDING_FINISH, ASSIGN_FINISH);
}

// The select callback of the generals' list: every row may be chosen.
uint8_t opeu_commander_always(uint8_t*, HostEvent, uint16_t, uint8_t) {
  return 1;
}

// The general sent on leave thanks the commander.
void opeu_rest_thanks(uint8_t ds[65536], HostEvent event, uint16_t, uint16_t person) {
  Call(ds, event, PersonnelService::MESSAGE, Near(Data(ds), Data(ds).rest_thanks_text), person);
}

// The appointed general accepts, and the appointment is shown.
void opeu_assign_confirm(uint8_t ds[65536], HostEvent event, uint16_t segment, uint16_t person) {
  auto const& data{ Data(ds) };
  Call(ds, event, PersonnelService::MESSAGE, Near(data, data.assign_thanks_text), person);
  Call(ds, event, PersonnelService::DAILY_SHOW, ASSIGN_SHOW, data.current_side, FINISH_OFFSET, segment);
}

// The select callback of the divisions' list: marks or unmarks the listed division for its commander to be put in
// charge, when it is in the field and its selected regiment may be selected.
uint8_t opeu_division_toggle(uint8_t ds[65536], HostEvent event, uint16_t, uint8_t) {
  auto& data{ Data(ds) };
  auto& group{ GroupOf(data, data.current_side, data.list_row) };
  if (group.flags & Bits(GroupFlag::READY))
    if (Call(ds, event, PersonnelService::UNIT_SELECTABLE, Call(ds, event, PersonnelService::GROUP_SELECTED, Near(data, group)))) group.flags ^= Bits(GroupFlag::AI_MARK);
  return 0;
}

// Asks which division commander to put in charge.
void opeu_division_commander_menu(uint8_t ds[65536], HostEvent event, uint16_t segment) {
  auto& data{ Data(ds) };
  SetListPredicate(data.division_dialog, TOGGLE_OFFSET, segment);
  Call(ds, event, PersonnelService::MESSAGE, Near(data, data.division_charge_prompt), data.context_record2);
  ShowList(ds, event, data.division_dialog);
}

// Fills general_list with the side's generals who are present, hold no staff post, lead no division and are not on
// leave; returns how many.
uint8_t opeu_idle_commanders_fill(uint8_t ds[65536], HostEvent, uint16_t) {
  auto&   data{ Data(ds) };
  uint8_t count{ 0 };
  for (auto& general : std::span{ data.people }.subspan(data.current_side * PEOPLE_PER_SIDE, PEOPLE_PER_SIDE))
    if (general.status != PERSON_VACANT && !(general.role & BUSY_ROLES) && !(general.assignment & Bits(Assignment::DIVISION)) && !opeu_get_record_counter(reinterpret_cast<uint8_t*>(&general)))
      data.general_list[count++] = Near(data, general);
  return count;
}

// Assigns free generals while there are any: one is chosen from the list, then the post, from a menu of the four
// divisions (with their commanders; a division not in the field cannot be chosen) and the operations and
// intelligence posts. A division's old commander leaves it; a staff post's old holder steps down.
void opeu_commander_assign(uint8_t ds[65536], HostEvent event, uint16_t segment) {
  auto& data{ Data(ds) };
  bool  assigned{ false };
  while (FillGenerals(ds, event, segment) != 1) {
    SetListPredicate(data.people_dialog, ALWAYS_OFFSET, segment);
    Call(ds, event, PersonnelService::MESSAGE, Near(data, data.assign_who_prompt), data.context_record2);
    if (!ShowList(ds, event, data.people_dialog)) {
      Call(ds, event, PersonnelService::PANEL);
      return;
    }
    auto const general{ data.general_list[data.list_row] };
    uint16_t   holders[POSTS];
    for (uint8_t group = 0; group < GROUPS_PER_SIDE; group++) holders[group] = opeu_find_group_record(ds, data.current_side, group);
    holders[4] = opeu_find_side_record(ds, data.current_side, std::to_underlying(StaffRole::OPERATIONS_CHIEF));
    holders[5] = opeu_find_side_record(ds, data.current_side, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF));
    // A division's line: its label, or a dash when it is not in the field, and its commander's name if any.
    uint16_t const      no_holder[GROUPS_PER_SIDE]{ Near(data, data.no_holder_1), Near(data, data.no_holder_2), Near(data, data.no_holder_3), Near(data, data.no_holder_4) };
    uint16_t const      formats[POSTS]{ Near(data, data.post_format_1), Near(data, data.post_format_2), Near(data, data.post_format_3),
                                        Near(data, data.post_format_4), Near(data, data.post_format_5), Near(data, data.post_format_6) };
    ServiceLocals const texts{ POSTS * POST_TEXT_BYTES + ASSIGN_PROMPT_BYTES };
    for (uint8_t post = 0; post < POSTS; post++) {
      auto const text{ texts.At(static_cast<uint16_t>(post * POST_TEXT_BYTES)) };
      if (post < GROUPS_PER_SIDE) {
        auto const fielded{ GroupOf(data, data.current_side, post).flags & Bits(GroupFlag::READY) };
        Call(ds, event, PersonnelService::FORMAT, text, formats[post], fielded ? data.post_labels[post] : Near(data, data.inactive_division_label), holders[post] ? holders[post] : no_holder[post]);
      } else {
        Call(ds, event, PersonnelService::FORMAT, text, formats[post], data.post_labels[post], holders[post]);
      }
      data.assign_menu[post].label = text;
    }
    Call(ds, event, PersonnelService::PANEL);
    auto const prompt{ texts.At(POSTS * POST_TEXT_BYTES) };
    Call(ds, event, PersonnelService::FORMAT, prompt, Near(data, data.assign_where_prompt), general);
    for (;;) {
      uint16_t choice{ Call(ds, event, PersonnelService::ASK, prompt, POSTS, data.context_record2, Near(data, data.assign_menu)) };
      if (choice == MENU_CANCELLED) break;
      if (choice <= GROUPS_PER_SIDE) {
        --choice;
        // The division by its index from the side's first, as the menu's choice gives it.
        auto const& group{ Record<Group>(data, static_cast<uint16_t>(GROUPS_AT + static_cast<uint16_t>(data.current_side * GROUPS_PER_SIDE + choice) * sizeof(Group))) };
        if (!(group.flags & Bits(GroupFlag::READY))) continue;
        auto const old{ opeu_find_group_record(ds, data.current_side, static_cast<uint8_t>(choice)) };
        if (old) opeu_set_record_counter(ds + old, 0);
        opeu_person_set_group(ds, general, static_cast<uint8_t>(choice));
        data.context_group_record = opeu_find_group_record(ds, data.current_side, data.current_group);
      } else {
        auto const role{ choice == 5 ? StaffRole::OPERATIONS_CHIEF : StaffRole::INTELLIGENCE_CHIEF };
        auto const old{ opeu_find_side_record(ds, data.current_side, std::to_underlying(role)) };
        if (old) opeu_person_set_role(ds, old, std::to_underlying(StaffRole::NONE));
        opeu_person_set_role(ds, general, std::to_underlying(role));
        data.context_record2 = opeu_find_side_record(ds, data.current_side, std::to_underlying(StaffRole::OPERATIONS_CHIEF));
        data.context_record3 = opeu_find_side_record(ds, data.current_side, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF));
      }
      opeu_assign_confirm(ds, event, segment, general);
      break;
    }
    assigned = true;
  }
  if (!assigned) Call(ds, event, PersonnelService::MESSAGE, Near(data, data.none_available_text), data.context_record3);
  Call(ds, event, PersonnelService::PANEL);
}

// Sends free generals on leave while there are any: one is chosen from the list and asked for how many days (the
// answer plus one).
void opeu_commander_rest(uint8_t ds[65536], HostEvent event, uint16_t segment) {
  ServiceLocals const locals{ REST_PROMPT_BYTES };
  auto&               data{ Data(ds) };
  auto const          prompt{ locals.At(0) };
  bool                rested{ false };
  for (; FillGenerals(ds, event, segment) != 1; rested = true) {
    SetListPredicate(data.people_dialog, ALWAYS_OFFSET, segment);
    Call(ds, event, PersonnelService::PANEL);
    Call(ds, event, PersonnelService::MESSAGE, Near(data, data.rest_who_prompt), data.context_record2);
    if (!ShowList(ds, event, data.people_dialog)) return;
    auto const general{ data.general_list[data.list_row] };
    Call(ds, event, PersonnelService::FORMAT, prompt, Near(data, data.rest_days_prompt), general);
    auto const days{ Call(ds, event, PersonnelService::ASK, prompt, REST_LAST, data.context_record2, 0) };
    if (days != MENU_CANCELLED) {
      opeu_rest_thanks(ds, event, segment, general);
      opeu_set_record_counter(ds + general, static_cast<uint8_t>(days + 1));
    }
  }
  if (!rested) Call(ds, event, PersonnelService::MESSAGE, Near(data, data.none_to_rest_text), data.context_record3);
}

// Turn action 4: the personnel orders menu, until it is cancelled.
void opeu_turn_action_4(uint8_t ds[65536], HostEvent event, uint16_t) {
  auto& data{ Data(ds) };
  for (uint16_t choice; (choice = Call(ds, event, PersonnelService::ASK, Near(data, data.personnel_prompt), ORDERS_LAST, data.context_record2, Near(data, data.personnel_menu))) != MENU_CANCELLED;) {
    auto const& action{ data.personnel_actions[choice - 1] };
    Call(ds, event, PersonnelService::RUN, action.segment, action.offset);
  }
}

uint16_t svc_persmenu(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r{ 0 };
  switch (static_cast<PersonnelService>(id)) {
  case PersonnelService::FORMAT:  opeu_request_format(ds, { args, count }); break;
  case PersonnelService::MESSAGE: opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case PersonnelService::PANEL:   opeu_title_panel_begin(ds, (const DialogHost*)port_dialogs_host()); break;
  case PersonnelService::DAILY_SHOW:
    opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint32_t)Arg(args, count, 3) << 16 | Arg(args, count, 2));
    break;
  case PersonnelService::PENDING_FINISH:  opeu_pending_finish(ds, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  case PersonnelService::LIST:            r = opeu_list_dialog(ds, svc_listdlg, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case PersonnelService::ASK:             r = opeu_delay_dialog(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case PersonnelService::GROUP_SELECTED:  r = opeu_find_group_selected_unit(ds, Arg(args, count, 0)); break;
  case PersonnelService::UNIT_SELECTABLE: r = opeu_unit_selection_eligible(ds, Arg(args, count, 0)); break;
  case PersonnelService::RUN:             r = (uint16_t)port_call(ds, (uint32_t)Arg(args, count, 0) * 16u + Arg(args, count, 1), nullptr, 0); break;
  default:                                port_unhandled("persmenu", id, count);
  }
  return r;
}

auto port_routine_opeu_commander_always(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_commander_always(ds, svc_persmenu, PERSONNEL_SEGMENT, (uint8_t)Arg(args, count, 0));
}

auto port_routine_opeu_commander_assign(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_commander_assign(ds, svc_persmenu, PERSONNEL_SEGMENT);
  return 0;
}

auto port_routine_opeu_commander_rest(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_commander_rest(ds, svc_persmenu, PERSONNEL_SEGMENT);
  return 0;
}

auto port_routine_opeu_division_commander_menu(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_division_commander_menu(ds, svc_persmenu, PERSONNEL_SEGMENT);
  return 0;
}

auto port_routine_opeu_division_toggle(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_division_toggle(ds, svc_persmenu, PERSONNEL_SEGMENT, (uint8_t)Arg(args, count, 0));
}

auto port_routine_opeu_personnel_finish(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_personnel_finish(ds, svc_persmenu, PERSONNEL_SEGMENT);
  return 0;
}

auto port_routine_opeu_turn_action_4(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_turn_action_4(ds, svc_persmenu, PERSONNEL_SEGMENT);
  return 0;
}
