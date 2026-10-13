// Data segment DS:8396..9111 (3452 bytes). Unit order, report and assignment messages with 12-byte menu-item tables.
#include "data/dgorders.hpp"
#include "data/image.hpp"

#include <array>
#include <cstddef>

namespace opeu::game::detail::dgorders {

auto InitialOrderText(DataSegment& data) -> void {
  SetText(data.casualty_heading, "\033C%c%s %s\012%s\012\033C7");
  SetText(data.casualty_cut_selected, "...\033W1...\033W1...\033S");
  SetText(data.casualty_cut, "Communications with %s were cut.\033S");
  SetText(data.casualty_loss_format, "suffered %d loss.\033S");
  SetText(data.casualty_losses_format, "suffered %d losses.\033S");
  SetText(data.casualty_wiped, "was wiped out.\033S");
  SetText(data.casualty_no_damage, "suffered no damage.\033S");
  SetText(data.damage_supply_format, "\033C%c%s\012\033C7%s Destroyed %u\012%s Remaining %u\033S");
  SetText(data.damage_title, "City damage report:\033S");
  SetText(data.damage_heading, "\033C%c%s's\033C7\012damage report, sir:\033S");
  SetText(data.damage_element_format, "\033C%c%s %s\012\033C7");
  SetText(data.damage_none, "suffered no damage.\033S");
  SetText(data.damage_loss_one, "suffered %d loss.\033S");
  SetText(data.damage_loss_many, "suffered %d losses.\033S");
  SetText(data.damage_endurance_format, "\033C%c%s's endurance\033C7 fell to %d (-%d).\033S");
  data.category_columns = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x03, 0x01, 0x01 };
  data.ai_categories    = { 0x00, 0x01, 0x02, 0x03, 0x04 };
  SetText(data.target_text, "Set a target, sir.");
  SetText(data.target_two_line_format, "%s\012%s?");
  SetText(data.target_one_line_format, "%s %s?");
  SetText(data.target_cell_format, "Move to (%d-%d)?");
  SetText(data.target_cancel_text, "Should we cancel this\012mission?");
  SetText(data.cancel_orders_prompt, "Cancel orders?");
  SetText(data.order_combat_move, "Move   ");
  SetText(data.order_combat_occupy, "Occupy ");
  SetText(data.order_combat_attack, "Attack ");
  SetText(data.order_combat_defend, "Defend ");
  SetText(data.order_combat_destroy, "Destroy");
  SetText(data.order_combat_supply, "Supply ");
  SetText(data.order_combat_pursue, "Pursue ");
  SetText(data.order_combat_rest, "Rest   ");
  SetText(data.order_engineer_move, "Move   ");
  SetText(data.order_engineer_mine, "Mine   ");
  SetText(data.order_engineer_bridge, "Bridge ");
  SetText(data.order_engineer_destroy, "Destroy");
  SetText(data.order_engineer_supply, "Supply ");
  SetText(data.order_engineer_pursue, "Pursue ");
  SetText(data.order_engineer_rest, "Rest   ");
  SetText(data.order_repair_move, "Move   ");
  SetText(data.order_repair_supply, "Supply ");
  SetText(data.order_repair_repair, "Repair ");
  SetText(data.order_repair_pursue, "Pursue ");
  SetText(data.order_repair_rest, "Rest   ");
  SetText(data.order_supply_move, "Move   ");
  SetText(data.order_supply_supply, "Supply ");
  SetText(data.order_supply_pursue, "Pursue ");
  SetText(data.order_supply_rest, "Rest   ");
  SetText(data.supply_choice_city, "City    ");
  SetText(data.supply_choice_regiment, "Regiment");
  SetText(data.repair_choice_city_weapons, "City Weapons");
  SetText(data.repair_choice_rgmt_weapons, "Rgmt Weapons");
  // clang-format off
  data.order_menus = { {
    { {
      { 0x0032, 0x00a3, 0x0009, 0x0700, 0x8578, 0x8001 },
      { 0x0039, 0x00a3, 0x0009, 0x0700, 0x8580, 0x8002 },
      { 0x0032, 0x00bb, 0x0009, 0x0700, 0x8588, 0x8003 },
      { 0x0039, 0x00bb, 0x0009, 0x0700, 0x8590, 0x8004 },
      { 0x0032, 0x00d3, 0x0009, 0x0700, 0x8598, 0x8005 },
      { 0x0039, 0x00d3, 0x0009, 0x0700, 0x85a0, 0x8006 },
      { 0x0032, 0x00eb, 0x0009, 0x0700, 0x85a8, 0x8007 },
      { 0x0039, 0x00eb, 0x0009, 0x0700, 0x85b0, 0x8008 },
    } },
    { {
      { 0x0032, 0x00a3, 0x0009, 0x0700, 0x85b8, 0x8001 },
      { 0x0039, 0x00a3, 0x0009, 0x0700, 0x85c0, 0x8002 },
      { 0x0032, 0x00bb, 0x0009, 0x0700, 0x85c8, 0x8003 },
      { 0x0039, 0x00bb, 0x0009, 0x0700, 0x85d0, 0x8004 },
      { 0x0032, 0x00d3, 0x0009, 0x0700, 0x85d8, 0x8005 },
      { 0x0039, 0x00d3, 0x0009, 0x0700, 0x85e0, 0x8006 },
      { 0x0039, 0x00eb, 0x0009, 0x0700, 0x85e8, 0x8007 },
    } },
    { {
      { 0x0032, 0x00a3, 0x0009, 0x0700, 0x85f0, 0x8001 },
      { 0x0039, 0x00a3, 0x0009, 0x0700, 0x85f8, 0x8002 },
      { 0x0032, 0x00bb, 0x0009, 0x0700, 0x8600, 0x8003 },
      { 0x0039, 0x00bb, 0x0009, 0x0700, 0x8608, 0x8004 },
      { 0x0039, 0x00d3, 0x0009, 0x0700, 0x8610, 0x8005 },
    } },
    { {
      { 0x0032, 0x00a3, 0x0009, 0x0700, 0x8618, 0x8001 },
      { 0x0039, 0x00a3, 0x0009, 0x0700, 0x8620, 0x8002 },
      { 0x0032, 0x00bb, 0x0009, 0x0700, 0x8628, 0x8003 },
      { 0x0039, 0x00d3, 0x0009, 0x0700, 0x8630, 0x8004 },
    } },
  } };
  // clang-format on
  // clang-format off
  data.supply_choices = { {
    { 0x002f, 0x00a3, 0x0008, 0x0700, 0x8638, 0x8001 },
    { 0x002f, 0x00bb, 0x0008, 0x0700, 0x8641, 0x8002 },
  } };
  // clang-format on
  // clang-format off
  data.repair_choices = { {
    { 0x002f, 0x00a3, 0x000d, 0x0700, 0x864a, 0x8001 },
    { 0x002f, 0x00bb, 0x000d, 0x0700, 0x8657, 0x8002 },
  } };
  // clang-format on
  // clang-format off
  data.order_spots = { {
    { 0x002f, 0x00a3 },
    { 0x0039, 0x00a3 },
    { 0x002f, 0x00bb },
    { 0x0039, 0x00bb },
    { 0x002f, 0x00d3 },
    { 0x0039, 0x00d3 },
    { 0x002f, 0x00eb },
    { 0x0039, 0x00eb },
  } };
  // clang-format on
  // clang-format off
  data.choice_spots = { {
    { 0x002f, 0x00a3 },
    { 0x002f, 0x00bb },
  } };
  // clang-format on
  data.issue_prompts = { { { "Your orders, sir?" } },
                         { { "Repair unit does not exist.\033E2" } },
                         { { "Please select the target coordinates, sir." } },
                         { { "Not an enemy city!\033E2" } },
                         { { "Fix which" } },
                         { { "Supply which" } },
                         { { "We cannot build a\012bridge there.\033E2" } },
                         { { "There is no bridge\012there.\033E2" } },
                         { { "No enemy there.\033E1" } },
                         { { "No regiment to pursue there, sir.\033E2" } },
                         { { "You cannot %s there.\033E2" } },
                         { { "Understood.\012I'll have them %s." } } };
  // clang-format off
  data.issue_order_codes = { {
    { { 0x00, 0x01, 0x02, 0x03, 0x07, 0x08, 0x0c, 0x0d } },
    { { 0x00, 0x04, 0x06, 0x07, 0x08, 0x0c, 0x0d, 0xff } },
    { { 0x00, 0x08, 0x0a, 0x0c, 0x0d, 0xff, 0xff, 0xff } },
    { { 0x00, 0x08, 0x0c, 0x0d, 0xff, 0xff, 0xff, 0xff } },
  } };
  // clang-format on
  SetText(data.awaiting_orders, "Awaiting your orders, sir.");
  data.action_prompts = { { { "Your orders, sir?" } },
                          { { "Repair unit does not exist.\033E2" } },
                          { { "Please select the target coordinates, sir." } },
                          { { "Not an enemy city!\033E2" } },
                          { { "Fix which" } },
                          { { "Supply which" } },
                          { { "We cannot build a\012bridge there.\033E2" } },
                          { { "There is no bridge\012there.\033E2" } },
                          { { "No enemy there.\033E1" } },
                          { { "No regiment to pursue there, sir.\033E2" } },
                          { { "You cannot %s there.\033E2" } },
                          { { "Understood.\012I'll have them %s." } } };
  // clang-format off
  data.action_order_codes = { {
    { { 0x00, 0x01, 0x02, 0x03, 0x07, 0x08, 0x0c, 0x0d } },
    { { 0x00, 0x04, 0x06, 0x07, 0x08, 0x0c, 0x0d, 0xff } },
    { { 0x00, 0x08, 0x0a, 0x0c, 0x0d, 0xff, 0xff, 0xff } },
  } };
  // clang-format on
  SetText(data.select_division, "Select a Division, sir.");
  SetText(data.select_order, "Select an order, sir");
  SetText(data.order_whole_division, "Issue an order to my\012entire division, sir?");
  SetText(data.select_regiment, "Select a Regiment, sir.");
  SetText(data.stock_header, "\033C7\033B1        Garrison    City     Set");
  SetText(data.stock_fuel_line, "\012\033C7 Fuel     %6u  %6u  \033C3%6u");
  SetText(data.stock_ammo_line, "\012\033C7 Ammo     %6u  %6u  \033C3%6u");
  SetText(data.stock_food_line, "\012\033C7 Food     %6u  %6u  \033C3%6u\033C7\033B0");
  SetText(data.stock_question_short, "How much %s should we\012store here");
  SetText(data.stock_question, "How much %s should we\012store in this city,\012sir(0-60000)");
  SetText(data.stock_fuel, "Fuel");
  SetText(data.stock_ammo, "Ammo");
  SetText(data.stock_food, "Food");
  SetText(data.stock_no_cities, "We have no cities.\033E3");
  SetText(data.stock_select_city, "Select a city.");
  SetText(data.transfer_which_division, "Transfer command to which division?");
  SetText(data.transfer_which_regiment, "Which regiment?");
  SetText(data.transfer_which_group, "Which group?");
  SetText(data.transfer_hour, "hour");
  SetText(data.transfer_hours_text, "hours");
  SetText(data.transfer_confirm, "It will take about\012%u %s to transfer.\012Is this OK?");
  SetText(data.rest_thanks_text, "Thank you very much, sir!\033W2");
  SetText(data.assign_thanks_text, "You can count on me, sir!");
  SetText(data.division_charge_prompt, "Which division commander do you want to put in charge?");
  SetText(data.post_div_1, "Div 1: ");
  SetText(data.post_div_2, "Div 2: ");
  SetText(data.post_div_3, "Div 3: ");
  SetText(data.post_div_4, "Div 4: ");
  SetText(data.post_ops_ofc, "Ops Ofc: ");
  SetText(data.post_intel, "Intel: ");
  SetText(data.none_available_text, "No one is available, sir.\033S");
  SetText(data.assign_who_prompt, "Who will you assign?");
  SetText(data.post_format_1, "%s%-8s");
  SetText(data.post_format_2, "%s%-8s");
  SetText(data.post_format_3, "%s%-8s");
  SetText(data.post_format_4, "%s%-8s");
  SetText(data.post_format_5, "%s%-8s");
  SetText(data.post_format_6, "%s%-8s");
  SetText(data.assign_where_prompt, "Where should %s\012be assigned, sir");
  // clang-format off
  data.assign_menu = { {
    { 0x002e, 0x0087, 0x0018, 0x0700, 0x8dba, 0x0001 },
    { 0x002e, 0x009f, 0x0018, 0x0700, 0x8dbb, 0x0002 },
    { 0x002e, 0x00b7, 0x0018, 0x0700, 0x8dbc, 0x0003 },
    { 0x002e, 0x00cf, 0x0018, 0x0700, 0x8dbd, 0x0004 },
    { 0x002e, 0x00e7, 0x0018, 0x0700, 0x8dbe, 0x0005 },
    { 0x002e, 0x00ff, 0x0018, 0x0700, 0x8dbf, 0x0006 },
  } };
  // clang-format on
  SetEach(std::to_array<std::uint16_t>({ 0x8dc0, 0x8dc8, 0x8dd0, 0x8dd8, 0x8de0, 0x8dea }), [&](std::size_t index, std::uint16_t value) { data.post_labels[index] = value; });
  data.inactive_division_label = { 0x20, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x20 };
  SetText(data.none_to_rest_text, "There are no commanders who can rest.\033E2");
  SetText(data.rest_who_prompt, "Who shall we rest?");
  SetText(data.rest_days_prompt, "How many days should\012Commander %s\012take leave");
  SetText(data.personnel_reassign, "Reassign");
  SetText(data.personnel_appoint, "Appoint");
  SetText(data.personnel_rest, "Rest");
  SetText(data.personnel_prompt, "Select an order");
  // clang-format off
  data.personnel_actions = { {
    PersonnelAction::REASSIGN,
    PersonnelAction::APPOINT,
    PersonnelAction::REST,
  } };
  // clang-format on
  // clang-format off
  data.personnel_menu = { {
    { 0x001e, 0x0018, 0x0008, 0x0700, 0x8f45, 0x8001 },
    { 0x001e, 0x0030, 0x0008, 0x0700, 0x8f4e, 0x8002 },
    { 0x001e, 0x0048, 0x0008, 0x0700, 0x8f56, 0x8003 },
  } };
  // clang-format on
  // clang-format off
  data.advice_tips = { {
    { 0x0d68, 0x3ad9 },
    { 0x0dad, 0x3ad9 },
    { 0x0e06, 0x3ad9 },
    { 0x0e38, 0x3ad9 },
    { 0x0e8d, 0x3ad9 },
    { 0x0ec3, 0x3ad9 },
    { 0x0f05, 0x3ad9 },
    { 0x0f4d, 0x3ad9 },
    { 0x0f7a, 0x3ad9 },
    { 0x0fd2, 0x3ad9 },
    { 0x101b, 0x3ad9 },
    { 0x1052, 0x3ad9 },
    { 0x109f, 0x3ad9 },
    { 0x10db, 0x3ad9 },
    { 0x1132, 0x3ad9 },
    { 0x1167, 0x3ad9 },
    { 0x11b3, 0x3ad9 },
    { 0x11f6, 0x3ad9 },
    { 0x1239, 0x3ad9 },
    { 0x1279, 0x3ad9 },
  } };
  // clang-format on
  SetText(data.advice_tactical_advice, "Tactical Advice");
  SetText(data.advice_regiment_status, "Regiment Status");
  SetText(data.advice_enemy_conditions, "Enemy Conditions");
  SetText(data.advice_tide_of_war, "Tide of War");
  SetText(data.advice_operation_hints, "Operation Hints");
  SetText(data.advice_prompt, "What are you interested in");
  // clang-format off
  data.advice_actions = { {
    AdviceAction::TACTICAL,
    AdviceAction::REGIMENT_STATUS,
    AdviceAction::ENEMY_CONDITIONS,
    AdviceAction::TIDE_OF_WAR,
    AdviceAction::OPERATION_HINTS,
  } };
  // clang-format on
  // clang-format off
  data.advice_menu = { {
    { 0x0025, 0x0018, 0x0012, 0x0700, 0x8ff8, 0x8001 }, // "Tactical Advice"
    { 0x0025, 0x0030, 0x0012, 0x0700, 0x9008, 0x8002 }, // "Regiment Status"
    { 0x0025, 0x0048, 0x0012, 0x0700, 0x9018, 0x8003 }, // "Enemy Conditions"
    { 0x0025, 0x0060, 0x0012, 0x0700, 0x9029, 0x8004 }, // "Tide of War"
    { 0x0025, 0x0078, 0x0012, 0x0700, 0x9035, 0x8005 }, // "Operation Hints"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.staff_operations, "Operations");
  SetText(data.staff_intelligence, "Intelligence");
  SetText(data.staff_prompt, "Which do you w");
  // clang-format off
  data.staff_actions = { {
    StaffAction::UNUSED,
    StaffAction::OPERATIONS,
    StaffAction::INTELLIGENCE,
  } };
  // clang-format on
  // clang-format off
  data.staff_menu = { {
    { 0x0025, 0x0018, 0x000d, 0x0700, 0x90bc, 0x8001 }, // "Operations"
    { 0x0025, 0x0030, 0x000d, 0x0700, 0x90c7, 0x8002 }, // "Intelligence"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
}
} // namespace opeu::game::detail::dgorders
