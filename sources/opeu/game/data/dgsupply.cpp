// Data segment DS:9112..9e0f (3326 bytes). Supply, victory-condition and battalion messages with menu-item tables.
#include "data/dgsupply.hpp"
#include "data/image.hpp"

#include <array>
#include <cstddef>

namespace opeu::game::detail::dgsupply {

auto InitialSupplyText(DataSegment& data) -> void {
  SetText(data.panel_fuel_ammo, "\033C7\033B1Fuel\033C3 %5u(%3d%%) \033C7Ammo "
                                "\033C3%5u(%3d%%)\033C7\033B0");
  SetText(data.panel_food_end, "\033C7\033B1Food\033C3 %5u(%3d%%) \033C7End\033C3  %2d/%2d\033C7\033B0");
  SetText(data.panel_garrison, "\033C7\033B1 ---- Garrison ---- \033C7\033B0");
  SetText(data.panel_stock_row, "\033C7\033B1%-10.10s \033C3%3u\033C7\033B0");
  SetText(data.panel_no_stock, "\033C3\033B1\033C7\033B0");
  // clang-format off
  data.stock_rows = { {
    { 0x002d, 0x00dd },
    { 0x003e, 0x00dd },
    { 0x002d, 0x00ee },
    { 0x003e, 0x00ee },
    { 0x002d, 0x00ff },
    { 0x003e, 0x00ff },
  } };
  // clang-format on
  SetText(data.panel_waiting, "Waiting");
  SetText(data.panel_orders, "\033C7\033B1Orders\033C3 %-7.7s   \033C7Fuel "
                             "\033C3%5u(%3d%%)\033C7\033B0");
  SetText(data.panel_target, "\033C7\033B1Target\033C3 %-9.9s \033C7Ammo\033C3 "
                             "%5u(%3d%%)\033C7\033B0");
  SetText(data.panel_fatigue, "\033C5\033B1Fatigue:  %-2d     \033C7Food\033C3 %5u(%3d%%)\033C7\033B0");
  SetText(data.panel_element_row, "\033C7\033B1%-11.11s\033C3 %3d\033C7\033B0");
  // clang-format off
  data.element_rows = { {
    { 0x002d, 0x00dd },
    { 0x003e, 0x00dd },
    { 0x002d, 0x00ee },
    { 0x003e, 0x00ee },
    { 0x002d, 0x00ff },
    { 0x003e, 0x00ff },
  } };
  // clang-format on
  SetText(data.text_92b6, " Division 1 ");
  SetText(data.text_92c3, " Division 2");
  SetText(data.text_92cf, " Division 3");
  SetText(data.text_92db, " Division 4");
  SetText(data.contact_prompt, "Which division would you like information about");
  SetText(data.contact_lost, "I can't establish contact with them, sir.\033S");
  // clang-format off
  data.contact_menu = { {
    { 0x0001, 0x0000, 0x000d, 0x0700, 0x92b6, 0x8001 }, // " Division 1 "
    { 0x000f, 0x0000, 0x000d, 0x0700, 0x92c3, 0x8002 }, // " Division 2"
    { 0x001d, 0x0000, 0x000d, 0x0700, 0x92cf, 0x8003 }, // " Division 3"
    { 0x002b, 0x0000, 0x000d, 0x0700, 0x92db, 0x8004 }, // " Division 4"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.people_here, "Here it is, sir.");
  SetText(data.people_none, "No enemy commanders\033E2");
  SetText(data.text_93a8, "Friendly");
  SetText(data.text_93b1, "Enemy");
  data.people_prompt = { 0x57, 0x68, 0x69, 0x63, 0x68, 0x20, 0x63, 0x6f, 0x6d, 0x6d, 0x61, 0x6e, 0x64, 0x65, 0x72,
                         0x73, 0x20, 0x77, 0x69, 0x6c, 0x6c, 0x0a, 0x79, 0x6f, 0x75, 0x20, 0x76, 0x69, 0x65, 0x77 };
  // clang-format off
  data.people_actions = { {
    { 0x0656, 0x1fe6 },
    { 0x06ae, 0x1fe6 },
  } };
  // clang-format on
  // clang-format off
  data.people_menu = { {
    { 0x0025, 0x0018, 0x0009, 0x0700, 0x93a8, 0x8001 }, // "Friendly"
    { 0x0025, 0x0030, 0x0009, 0x0700, 0x93b1, 0x8002 }, // "Enemy"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.city_prompt, "Which city will you view?");
  SetText(data.city_format, "%s");
  SetText(data.city_none_own, "We have no friendly cities!\033E2");
  SetText(data.city_none_enemy, "There are no enemy cities!\033E2");
  SetText(data.text_945c, "Friendly");
  SetText(data.text_9465, "Enemy");
  SetEach(std::to_array<std::uint16_t>({ 0x6857, 0x6369, 0x2068, 0x6973, 0x6564, 0x7720, 0x6c69, 0x206c, 0x6f79, 0x0a75, 0x6976, 0x7765, 0x202c, 0x6973, 0x0072 }),
          [&](std::size_t index, std::uint16_t value) { data.city_side_prompt[index] = value; });
  // clang-format off
  data.city_actions = { {
    { 0x082e, 0x1fe6 },
    { 0x085e, 0x1fe6 },
  } };
  // clang-format on
  // clang-format off
  data.city_side_menu = { {
    { 0x0025, 0x0018, 0x0009, 0x0700, 0x945c, 0x8001 }, // "Friendly"
    { 0x0025, 0x0030, 0x0009, 0x0700, 0x9465, 0x8002 }, // "Enemy"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.regiment_prompt, "Which regiment will you view, sir?");
  SetText(data.regiment_format, "%s");
  SetText(data.view_division_prompt, "Which division will you view?");
  SetText(data.units_none_enemy, "No enemy units sighted.\033E3");
  SetText(data.text_9515, "Friendly");
  SetText(data.text_951e, "Enemy");
  SetEach(std::to_array<std::uint16_t>({ 0x6857, 0x6369, 0x2068, 0x6e69, 0x6f66, 0x7720, 0x6c69, 0x206c, 0x6f79, 0x0a75, 0x6976, 0x7765, 0x202c, 0x6973, 0x0072 }),
          [&](std::size_t index, std::uint16_t value) { data.info_prompt[index] = value; });
  // clang-format off
  data.info_actions = { {
    { 0x09d6, 0x1fe6 },
    { 0x0a7a, 0x1fe6 },
  } };
  // clang-format on
  // clang-format off
  data.info_menu = { {
    { 0x0025, 0x0018, 0x0009, 0x0700, 0x9515, 0x8001 }, // "Friendly"
    { 0x0025, 0x0030, 0x0009, 0x0700, 0x951e, 0x8002 }, // "Enemy"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.weapons_prompt, "Which weapons will you view?");
  SetText(data.weapons_here, "Here it is, sir.\033S");
  SetText(data.text_959e, "Friendly");
  SetText(data.text_95a7, "Enemy");
  SetEach(std::to_array<std::uint16_t>({ 0x6857, 0x6369, 0x2068, 0x6977, 0x6c6c, 0x7920, 0x756f, 0x7620, 0x6569, 0x0077 }),
          [&](std::size_t index, std::uint16_t value) { data.arms_prompt[index] = value; });
  // clang-format off
  data.arms_actions = { {
    { 0x0ba6, 0x1fe6 },
    { 0x0be4, 0x1fe6 },
  } };
  // clang-format on
  // clang-format off
  data.arms_menu = { {
    { 0x0025, 0x0018, 0x0009, 0x0700, 0x959e, 0x8001 }, // "Friendly"
    { 0x0025, 0x0030, 0x0009, 0x0700, 0x95a7, 0x8002 }, // "Enemy"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.victory_title, "Victory Conditions");
  SetText(data.victory_format, "\033B1\033C7%s\033B0\033C7");
  SetText(data.victory_orders, "Here are our orders, sir.\033S");
  SetText(data.text_962c, "Divisions");
  SetText(data.text_9636, "Officers");
  SetText(data.text_963f, "Cities");
  SetText(data.text_9646, "Units");
  SetText(data.text_964c, "Armaments");
  SetText(data.text_9656, "Victory Conditions");
  SetEach(std::to_array<std::uint16_t>({ 0x6857, 0x6369, 0x2068, 0x6e69, 0x6f66, 0x6420, 0x206f, 0x6f79, 0x2075, 0x6977, 0x6873, 0x740a, 0x206f, 0x6976, 0x7765, 0x202c, 0x6973, 0x0072 }),
          [&](std::size_t index, std::uint16_t value) { data.view_main_prompt[index] = value; });
  // clang-format off
  data.view_main_actions = { {
    { 0x04fa, 0x1fe6 },
    { 0x0724, 0x1fe6 },
    { 0x0894, 0x1fe6 },
    { 0x0b02, 0x1fe6 },
    { 0x0c28, 0x1fe6 },
    { 0x0c64, 0x1fe6 },
  } };
  // clang-format on
  // clang-format off
  data.view_main_menu = { {
    { 0x0025, 0x0018, 0x0012, 0x0700, 0x962c, 0x8001 }, // "Divisions"
    { 0x0025, 0x0030, 0x0012, 0x0700, 0x9636, 0x8002 }, // "Officers"
    { 0x0025, 0x0048, 0x0012, 0x0700, 0x963f, 0x8003 }, // "Cities"
    { 0x0025, 0x0060, 0x0012, 0x0700, 0x9646, 0x8004 }, // "Units"
    { 0x0025, 0x0078, 0x0012, 0x0700, 0x964c, 0x8005 }, // "Armaments"
    { 0x0025, 0x0090, 0x0012, 0x0700, 0x9656, 0x8006 }, // "Victory Conditions"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.ops_chief_format, "\033M4\033C6Ops Chief: %s\033M0\033C7\033RU");
  SetText(data.number_range_format, "(%u-%u)?");
  data.supply_unlimited = 0xea60;
  data.supply_panel_x   = 0x0018;
  data.supply_panel_y   = 0x008c;
  SetText(data.supply_name_format, "\033C7\033B0\033Y0%.10s");
  SetText(data.supply_name_text, "%.10s\033C7\033B0");
  SetText(data.supply_arrow_none, "--");
  SetText(data.supply_arrow_left, "<-");
  SetText(data.supply_arrow_right, "->");
  SetText(data.supply_row_left, "\033C3\033B0%5u");
  SetText(data.supply_row_right, "%5u");
  SetText(data.supply_row_end, "\033C7\033B0");
  SetEach(std::to_array<std::uint16_t>({ 0x9741, 0x9744, 0x9747 }), [&](std::size_t index, std::uint16_t value) { data.supply_arrows[index] = value; });
  data.supply_prompt = { 0x41, 0x6c, 0x6c, 0x6f, 0x63, 0x61, 0x74, 0x65, 0x20, 0x73, 0x75, 0x70, 0x70, 0x6c, 0x69, 0x65, 0x73, 0x2c, 0x20, 0x73, 0x69, 0x72, 0x2e };
  SetText(data.supply_continue, "Continue?");
  SetText(data.tank_suffix, " Tank");
  // clang-format off
  data.nation_codes = { {
    { { "GER" } },
    { { "ITY" } },
    { { "USA" } },
    { { "ENG" } },
    { { "FRN" } },
    { { "RUS" } },
    { { "USA" } },
  } };
  // clang-format on
  SetText(data.nation_code_other, "    ");
  SetText(data.text_97af, ".%+");
  data.supply_type_list = {
    0x0000, 0x001c, 0x0020, 0x00f9, 0x97af, 0x0d, { 0x0dfe, 0x20b3 },
          { 0x0dfa, 0x20b3 }
  };
  SetText(data.supply_type_header, " Battalion     NAT  #OK (dmg)");
  SetText(data.supply_type_row, "\033C%c %-12.12s %4s  %4u (%2u)\033C7");
  SetText(data.text_9805, "6%%");
  SetText(data.supply_returned_slot, "\033C%c -- -- -- -- -- -- --           \033C7");
  data.supply_slot_list = {
    0x002c, 0x001c, 0x004e, 0x00f9, 0x9805, 0x0d, { 0x0fac, 0x20b3 },
          { 0x0ed8, 0x20b3 }
  };
  SetText(data.supply_slot_header, " Battalion             NAT  #OK");
  SetText(data.supply_slot_row, "\033C%c %-20s %4s %4d \033C7");
  SetText(data.supply_empty_slot, "\033C%c -- -- -- -- -- -- -- \033C7");
  SetText(data.alloc_amount_format, "\033C%c%4u (%2u)\033C7");
  SetText(data.alloc_first_format, "\033B3\033Y0%.10s\033B0");
  SetText(data.alloc_second_format, "\033B5\033Y0%.10s\033B0");
  SetText(data.alloc_heading, " Battalion    NAT  #OK (dmg)");
  SetText(data.alloc_row_format, "\033C%c %-12.12s  %3s\033C7");
  SetText(data.alloc_name_format, "\033M4\033C6%s\033M0\033C7");
  SetText(data.alloc_stats_format, "\033R<\033C7\033M4AP/AT  \033C%c%2d\033C7/\033C%c%2d\033C7\012Dfns  "
                                   "   \033C%c%3d\033C7\012Spd      \033C%c%3d\033C7\012M.Rng    "
                                   "\033C%c%3d\033C7\012G.Rng    \033C%c%3d\033RU\033C7\033M0");
  SetText(data.regiment_hq_prompt, "Choose the Regiment HQ\012for %s.");
  SetText(data.organize_none, "No units can be reorganized.\033E1");
  SetText(data.organize_which, "Organize which Regiment?");
  SetText(data.organize_confirm_format, "Organize the force in\012%s?");
  SetText(data.organize_no_arms, "No arms to equip\033E1");
  SetText(data.organize_no_neighbor, "No adjoining regiment\012with which to organize.\033E2");
  SetText(data.organize_with, "Organize with which regiment?");
  SetText(data.organize_full, "You cannot add any more\012battalions,sir!");
  SetText(data.organize_dispatch, "Dispatch where?");
  SetText(data.organize_complete, "We will complete\012reorganization and\012destroy damaged "
                                  "arms.\012Is this OK?");
  SetText(data.organize_cancel, "Cancel all changes?");
  SetText(data.organize_select, "Select forces, sir.");
  SetText(data.organize_number, "Number to assign, sir?\012");
  SetText(data.rq_no_more_text, "You cannot increase your %s any more.\033S");
  SetText(data.rq_cancel_text, "Your %s\012request has been sent.\012Cancel the request?");
  SetText(data.rq_filed_text, "I will file the request immediately.");
  SetText(data.rq_supplies_text, "Supplies");
  SetText(data.rq_supplies_title, "Supplies request");
  SetText(data.rq_supplies_where, "Which city would you\012like to receive\012the supplies?");
  SetText(data.rq_supplies_confirm, "%s.\012Is this correct, sir?");
  SetText(data.text_9c01, ".&'&&&&");
  data.rq_type_dialog = {
    0x000e, 0x0080, 0x0043, 0x0119, 0x9c01, 0x9f, { 0x01f6, 0x22f9 },
          { 0x01f2, 0x22f9 }
  };
  SetText(data.rq_type_header, "  Nat  AP/AT   Def   Spd  M.Rng G.Rng");
  SetText(data.rq_type_name, "%-13.13s");
  SetText(data.rq_type_row, "  %2s  %2d/%2d  %4d  %4d  %4d %4d ");
  SetText(data.rq_arms_text, "Arms");
  SetText(data.rq_arms_which, "What kind of battalion would you like sent?");
  SetText(data.rq_arms_where, "To which city shall we assign the battalion, sir?");
  SetText(data.rq_arms_confirm, "%s.\012Is this correct, sir?");
  SetText(data.text_9cec, ",&&&&");
  data.rq_regiment_list = {
    0x0015, 0x0080, 0x003b, 0x0119, 0x9cec, 0x21, { 0x04d2, 0x22f9 },
          { 0x04ce, 0x22f9 }
  };
  SetText(data.rq_unit_header, "  Men  Fuel  Ammo  Food");
  SetText(data.rq_unit_name, "%-12.12s");
  SetText(data.rq_unit_row, "  0/0     0     0     0 ");
  SetText(data.text_9d3f, "Infantry");
  SetText(data.text_9d48, "Mechanized");
  SetText(data.text_9d53, "Armored");
  SetText(data.text_9d5b, "Artillery");
  SetText(data.rq_reinforcements_text, "reinforcements");
  SetText(data.rq_regiment_text, "regiment");
  SetText(data.rq_battalion_text, "battalion");
  SetText(data.rq_which_regiment_text, "Which regiment should we call in, sir?");
  SetText(data.rq_regiments_text, "Regiments");
  SetText(data.rq_regiment_kind_text, "What kind of regiment,\012sir");
  // clang-format off
  data.rq_regiment_kind_menu = { {
    { 0x002f, 0x00a3, 0x000a, 0x0700, 0x9d3f, 0x8001 }, // "Infantry"
    { 0x002f, 0x00bb, 0x000a, 0x0700, 0x9d48, 0x8002 }, // "Mechanized"
    { 0x002f, 0x00d3, 0x000a, 0x0700, 0x9d53, 0x8003 }, // "Armored"
    { 0x002f, 0x00eb, 0x000a, 0x0700, 0x9d5b, 0x8004 }, // "Artillery"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
}
} // namespace opeu::game::detail::dgsupply
