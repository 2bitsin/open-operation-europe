// The personnel orders menu (turn action 4): putting a division commander in charge, assigning a free general to a
// division or a staff post, and sending a general on leave.
#include "core/persmenu.hpp"
#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
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

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "core/selunit.hpp"
#include "io/listdlg.hpp"
#include "io/waitkey.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

constexpr uint16_t ASSIGN_SHOW{ 0x17b };
constexpr uint8_t  ASSIGN_FINISH{ 3 };
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
auto ShowList(Game& game, ListWindow const& list) -> uint16_t {
  return opeu_list_dialog(game, list, LIST_OPTIONS);
}

// Lists the side's free generals and sizes the generals' list to them (one row more for the title).
auto FillGenerals(Game& game) -> uint8_t {
  auto& list{ Data(game).people_dialog };
  list.rows = opeu_idle_commanders_fill(game) + 1;
  return list.rows;
}

} // namespace

// The picture callback of the assignment's confirmation.
void opeu_personnel_finish(Game& game) {
  opeu_pending_finish(game, ASSIGN_FINISH);
}

// The select callback of the generals' list: every row may be chosen.
uint8_t opeu_commander_always(Game&, uint8_t) {
  return 1;
}

// The general sent on leave thanks the commander.
void opeu_rest_thanks(Game& game, uint16_t person) {
  opeu_control_message(game, Near(Data(game), Data(game).rest_thanks_text), person);
}

// The appointed general accepts, and the appointment is shown.
void opeu_assign_confirm(Game& game, uint16_t person) {
  auto const& data{ Data(game) };
  opeu_control_message(game, Near(data, data.assign_thanks_text), person);
  opeu_daily_show(game, ASSIGN_SHOW, data.current_side, Presenter::PERSONNEL_FINISH);
}

// The select callback of the divisions' list: marks or unmarks the listed division for its commander to be put in
// charge, when it is in the field and its selected regiment may be selected.
uint8_t opeu_division_toggle(Game& game, uint8_t) {
  auto& data{ Data(game) };
  auto& group{ GroupOf(data, data.current_side, data.list_row) };
  if (group.flags & Bits(GroupFlag::READY))
    if (opeu_unit_selection_eligible(game, opeu_find_group_selected_unit(game, Near(data, group)))) group.flags ^= Bits(GroupFlag::AI_MARK);
  return 0;
}

// Asks which division commander to put in charge.
void opeu_division_commander_menu(Game& game) {
  auto& data{ Data(game) };
  SetListPredicate(data.division_dialog, ListSelect::DIVISION_TOGGLE);
  opeu_control_message(game, Near(data, data.division_charge_prompt), data.context_record2);
  ShowList(game, data.division_dialog);
}

// Fills general_list with the side's generals who are present, hold no staff post, lead no division and are not on
// leave; returns how many.
uint8_t opeu_idle_commanders_fill(Game& game) {
  auto&   data{ Data(game) };
  uint8_t count{ 0 };
  for (auto& general : std::span{ data.people }.subspan(data.current_side * PEOPLE_PER_SIDE, PEOPLE_PER_SIDE))
    if (general.status != PERSON_VACANT && !(general.role & BUSY_ROLES) && !(general.assignment & Bits(Assignment::DIVISION)) && !opeu_get_record_counter(general))
      data.general_list[count++] = Near(data, general);
  return count;
}

// Assigns free generals while there are any: one is chosen from the list, then the post, from a menu of the four
// divisions (with their commanders; a division not in the field cannot be chosen) and the operations and
// intelligence posts. A division's old commander leaves it; a staff post's old holder steps down.
void opeu_commander_assign(Game& game) {
  auto& data{ Data(game) };
  bool  assigned{ false };
  while (FillGenerals(game) != 1) {
    SetListPredicate(data.people_dialog, ListSelect::ANY_GENERAL);
    opeu_control_message(game, Near(data, data.assign_who_prompt), data.context_record2);
    if (!ShowList(game, data.people_dialog)) {
      opeu_title_panel_begin(game);
      return;
    }
    auto const general{ data.general_list[data.list_row] };
    uint16_t   holders[POSTS];
    for (uint8_t group = 0; group < GROUPS_PER_SIDE; group++) holders[group] = opeu_find_group_record(game, data.current_side, group);
    holders[4] = opeu_find_side_record(game, data.current_side, std::to_underlying(StaffRole::OPERATIONS_CHIEF));
    holders[5] = opeu_find_side_record(game, data.current_side, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF));
    // A division's line: its label, or a dash when it is not in the field, and its commander's name if any.
    uint16_t const      no_holder[GROUPS_PER_SIDE]{ Near(data, data.no_holder_1), Near(data, data.no_holder_2), Near(data, data.no_holder_3), Near(data, data.no_holder_4) };
    uint16_t const      formats[POSTS]{ Near(data, data.post_format_1), Near(data, data.post_format_2), Near(data, data.post_format_3),
                                        Near(data, data.post_format_4), Near(data, data.post_format_5), Near(data, data.post_format_6) };
    ServiceLocals const texts{ POSTS * POST_TEXT_BYTES + ASSIGN_PROMPT_BYTES };
    for (uint8_t post = 0; post < POSTS; post++) {
      auto const text{ texts.At(static_cast<uint16_t>(post * POST_TEXT_BYTES)) };
      if (post < GROUPS_PER_SIDE) {
        auto const fielded{ GroupOf(data, data.current_side, post).flags & Bits(GroupFlag::READY) };
        opeu_request_format(
            game, text,
            std::array<FormatArgument, 3>{ formats[post], (fielded ? data.post_labels[post] : Near(data, data.inactive_division_label)), (holders[post] ? holders[post] : no_holder[post]) });
      } else {
        opeu_request_format(game, text, std::array<FormatArgument, 3>{ formats[post], data.post_labels[post], holders[post] });
      }
      data.assign_menu[post].label = text;
    }
    opeu_title_panel_begin(game);
    auto const prompt{ texts.At(POSTS * POST_TEXT_BYTES) };
    opeu_request_format(game, prompt, std::array<FormatArgument, 2>{ Near(data, data.assign_where_prompt), general });
    for (;;) {
      uint16_t choice{ opeu_delay_dialog(game, prompt, POSTS, data.context_record2, Near(data, data.assign_menu)) };
      if (choice == MENU_CANCELLED) break;
      if (choice <= GROUPS_PER_SIDE) {
        --choice;
        // The division by its index from the side's first, as the menu's choice gives it.
        auto const& group{ Record<Group>(data, static_cast<uint16_t>(GROUPS_AT + static_cast<uint16_t>(data.current_side * GROUPS_PER_SIDE + choice) * sizeof(Group))) };
        if (!(group.flags & Bits(GroupFlag::READY))) continue;
        auto const old{ opeu_find_group_record(game, data.current_side, choice) };
        if (old) opeu_set_record_counter(Record<Person>(Data(game), old), 0);
        opeu_person_set_group(game, general, choice);
        data.context_group_record = opeu_find_group_record(game, data.current_side, data.current_group);
      } else {
        auto const role{ choice == 5 ? StaffRole::OPERATIONS_CHIEF : StaffRole::INTELLIGENCE_CHIEF };
        auto const old{ opeu_find_side_record(game, data.current_side, std::to_underlying(role)) };
        if (old) opeu_person_set_role(game, old, std::to_underlying(StaffRole::NONE));
        opeu_person_set_role(game, general, std::to_underlying(role));
        data.context_record2 = opeu_find_side_record(game, data.current_side, std::to_underlying(StaffRole::OPERATIONS_CHIEF));
        data.context_record3 = opeu_find_side_record(game, data.current_side, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF));
      }
      opeu_assign_confirm(game, general);
      break;
    }
    assigned = true;
  }
  if (!assigned) opeu_control_message(game, Near(data, data.none_available_text), data.context_record3);
  opeu_title_panel_begin(game);
}

// Sends free generals on leave while there are any: one is chosen from the list and asked for how many days (the
// answer plus one).
void opeu_commander_rest(Game& game) {
  ServiceLocals const locals{ REST_PROMPT_BYTES };
  auto&               data{ Data(game) };
  auto const          prompt{ locals.At(0) };
  bool                rested{ false };
  for (; FillGenerals(game) != 1; rested = true) {
    SetListPredicate(data.people_dialog, ListSelect::ANY_GENERAL);
    opeu_title_panel_begin(game);
    opeu_control_message(game, Near(data, data.rest_who_prompt), data.context_record2);
    if (!ShowList(game, data.people_dialog)) return;
    auto const general{ data.general_list[data.list_row] };
    opeu_request_format(game, prompt, std::array<FormatArgument, 2>{ Near(data, data.rest_days_prompt), general });
    auto const days{ opeu_delay_dialog(game, prompt, REST_LAST, data.context_record2, 0) };
    if (days != MENU_CANCELLED) {
      opeu_rest_thanks(game, general);
      opeu_set_record_counter(Record<Person>(Data(game), general), days + 1);
    }
  }
  if (!rested) opeu_control_message(game, Near(data, data.none_to_rest_text), data.context_record3);
}

namespace {
// The personnel menu's routines, by the enumerator personnel_actions holds for the choice.
constexpr auto PERSONNEL_ACTIONS = DispatchRows<PersonnelAction, auto (*)(Game&)->void>({
    { PersonnelAction::REASSIGN, opeu_division_commander_menu },
    { PersonnelAction::APPOINT,  opeu_commander_assign        },
    { PersonnelAction::REST,     opeu_commander_rest          },
});
} // namespace

// Turn action 4: the personnel orders menu, until it is cancelled.
void opeu_turn_action_4(Game& game) {
  auto& data{ Data(game) };
  for (uint16_t choice; (choice = opeu_delay_dialog(game, Near(data, data.personnel_prompt), ORDERS_LAST, data.context_record2, Near(data, data.personnel_menu))) != MENU_CANCELLED;) {
    Dispatch(PERSONNEL_ACTIONS, data.personnel_actions.at(choice - 1u), game);
  }
}
