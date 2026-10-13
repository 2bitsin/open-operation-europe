// Data segment DS:537c..5bb1 (2102 bytes). Unit/location label formats, deputy name, army-summary rows, credits,
// roster headings and city names, with their far callback pointers.
#include "data/dglabels.hpp"
#include "data/image.hpp"

#include <array>
#include <cstddef>

namespace opeu::game::detail::dglabels {

auto InitialLabels(DataSegment& data) -> void {
  SetText(data.unit_name_format, "%s %2d");
  SetText(data.type_name_suffix, " Tank");
  SetText(data.location_name_format, "%2d-%2d");
  data.deputy_identity_ranges = { 0x6a, 0x6f, 0x70, 0x75 };
  SetText(data.deputy_name, "Deputy Cmdr");
  SetText(data.army_out, "\033B1\012Communications out!\033B0");
  SetText(data.army_row0, "\033B1\033C6Tanks\033C6%6ld \033C6Infnty\033C6%6ld "
                          "\033C5Fuel\033C5%4d%%");
  SetText(data.army_row1, "\033C6Armor\033C6%6ld \033C6Enginr\033C6%6ld \033C5Ammo\033C5%4d%%");
  SetText(data.army_row2, "\033C6Mech \033C6%6ld \033C6Supply\033C6%6ld \033C5Food\033C5%4d%%");
  SetText(data.army_row3, "\033C6Arty \033C6%6ld \033C6Repair\033C6%6ld");
  SetText(data.army_row4, "\033C7Bmbrs  %4d Cities    %2d Land %3d%%");
  SetText(data.army_row5, "\033C7Cargo  %4d Paratp  %4d Air  %3d%%");
  SetText(data.army_hq, "\033C7Army HQ in %s\033B0");
  SetText(data.army_person_format, "\033C%c%.8s\033C7");
  SetText(data.army_hq_label, "Army HQ");
  SetText(data.division_label_format, " Div \033C%c%d\033C7");
  SetText(data.division_person_format, "\033C%c%.8s\033C7");
  SetText(data.panel_name_format, "%-18s");
  SetText(data.panel_count_format, "\033C%c%3d\033C7");
  SetText(data.panel_regiment_assault_1, "Assault");
  SetText(data.panel_regiment_assault_2, "Assault");
  SetText(data.panel_regiment_engineer, "Engineer");
  SetText(data.panel_regiment_supply, "Supply");
  SetText(data.panel_regiment_format, "Rgmt \033C%c%d%d\033C7");
  SetText(data.panel_battle, "Battle");
  SetText(data.panel_string_format, "%s");
  data.panel_regiment_names = { 0x5514, 0x551c, 0x5524, 0x552d };
  data.credits_title        = { 0x1b, 0x4d, 0x34, 0x54, 0x68, 0x69, 0x73, 0x20, 0x67, 0x61, 0x6d, 0x65, 0x27, 0x73, 0x20, 0x63, 0x6f, 0x6e,
                                0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x20, 0x74, 0x65, 0x61, 0x6d, 0x20, 0x77, 0x61, 0x73, 0x3a };
  SetText(data.credits_main, "Paul Despe, Main Programmer");
  SetText(data.credits_support, "Colin Brady, Support Programming");
  SetText(data.credits_translation, "Kevin Snow and Yoko Yamamoto, Translation & Management\033M0");
  SetText(data.tbl_group_header, "  Cmdr   Name       Fuel   Ammo  Food");
  SetText(data.tbl_group_title, "%s");
  SetText(data.tbl_group_mark, "\033B8");
  SetText(data.tbl_group_division, " Division %-2d\033B0");
  SetText(data.tbl_group_empty, " --------");
  SetText(data.tbl_group_div, " Div ");
  SetText(data.tbl_group_army, " Army ");
  SetText(data.tbl_group_row, " %-6s  %-10.10s %3d%%  %3d%%   %3d%% ");
  SetText(data.tbl_group_blank, " ------  --------   ----   ----  ---- ");
  SetText(data.division_widths, ",(+&''");
  SetText(data.tbl_mission_header, " Content ");
  SetText(data.tbl_mission_label, " Mission %2d ");
  SetText(data.tbl_mission_place, "to (%3d-%3d)    ");
  SetText(data.tbl_mission_target, "%s       ");
  SetText(data.tbl_mission_row, " %s %s ");
  SetText(data.tbl_mission_blank, " ----------------                    ");
  data.division_dialog     = { 0x000f, 0x0080, 0x0044, 0x010e, 0x5697, 0x05, ListDraw::GROUPS, ListSelect::NONE };
  data.order_target_widths = { 0x2d, 0x3c };
  SetText(data.tbl_unit_header, "   Pos      Target    Orders  Fuel  Ammo  Food");
  SetText(data.tbl_unit_mark, "\033B8");
  SetText(data.tbl_unit_label, " %-10s \033B0");
  SetText(data.tbl_unit_empty, " ---------- ");
  SetText(data.tbl_unit_blank, "   -----     -----   -------  ----  ----  ---- ");
  SetText(data.tbl_unit_wait, "Wait");
  SetText(data.tbl_unit_row, " %8.8s  %8.8s  %-7s  %3d%%  %3d%%  %3d%% ");
  SetText(data.tbl_unit_hidden, " %8.8s   -------   ------  ----  ----  ---- ");
  data.order_target_list = { 0x0016, 0x0080, 0x0041, 0x0119, 0x5713, 0x05, ListDraw::MISSIONS, ListSelect::NONE };
  data.organize_widths   = { 0x2c, 0x2a, 0x2a, 0x29, 0x26, 0x26, 0x26 };
  SetText(data.tbl_order_header, "     Pos       Target   Orders  Fuel   Ammo   Food");
  SetText(data.tbl_order_label, " %-10s \033B0");
  data.organize_list   = { 0x0009, 0x0080, 0x0046, 0x0119, 0x5801, 0x21, ListDraw::UNITS, ListSelect::NONE };
  data.regiment_widths = { 0x2c, 0x2c, 0x2c, 0x26, 0x27, 0x27, 0x27 };
  SetText(data.tbl_element_header, " # OK   Dmg  Mrle  Trng  Fatg");
  SetText(data.tbl_element_mark, "\033B8");
  SetText(data.tbl_element_name, " %-19s\033B0");
  SetText(data.tbl_element_row, " %4d %4d    %2d    %2d    %2d ");
  data.regiment_dialog = { 0x0007, 0x0080, 0x0048, 0x0119, 0x585b, 0x21, ListDraw::ORDERS, ListSelect::NONE };
  data.choice_widths   = { 0x34, 0x26, 0x26, 0x26, 0x26, 0x26 };
  SetText(data.tbl_post_div1, "Div1");
  SetText(data.tbl_post_div2, "Div2");
  SetText(data.tbl_post_div3, "Div3");
  SetText(data.tbl_post_div4, "Div4");
  SetText(data.tbl_post_cmdr, "Cmdr ");
  SetText(data.tbl_post_ops, "Ops  ");
  SetText(data.tbl_post_intl, "Intl ");
  SetText(data.tbl_person_name, "%-11.11s");
  SetText(data.tbl_person_blank, "    ");
  SetText(data.tbl_person_rest, "Rest");
  SetText(data.tbl_person_hurt, "Hurt");
  data.choice_dialog = { 0x000e, 0x0080, 0x0042, 0x0119, 0x58c1, 0x07, ListDraw::ELEMENTS, ListSelect::NONE };
  SetEach(std::to_array<std::uint16_t>({ 0x58c8, 0x58cd, 0x58d2, 0x58d7, 0x58dc, 0x58e2, 0x58e8 }), [&](std::size_t index, std::uint16_t value) { data.tbl_post_names[index] = value; });
  data.people_widths = { 0x2d, 0x25, 0x25, 0x26, 0x26, 0x26, 0x26, 0x26, 0x27, 0x25, 0x26, 0x26 };
  SetText(data.tbl_type_header, "  Nat   AP/AT   Def  Spd  M.Rng  G.Rng");
  SetText(data.tbl_type_name, "%-13.13s");
  SetText(data.tbl_type_row, "  %2s   %2u/%2u  %4u %4u   %4u   %4u");
  data.people_dialog = { 0x0000, 0x0080, 0x004f, 0x0119, 0x5928, 0x15, ListDraw::STAFF, ListSelect::NONE };
  SetText(data.weapons_widths, ".&)%%''");
  data.tbl_supply_header = { 0x20, 0x20, 0x20, 0x20, 0x20, 0x46, 0x75, 0x65, 0x6c, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x41, 0x6d, 0x6d, 0x6f, 0xee, 0x20, 0x20, 0x20,
                             0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x46, 0x6f, 0x6f, 0x64, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x45, 0x6e, 0x64, 0x72, 0x63, 0x20 };
  SetText(data.tbl_supply_name, "%-11.11s");
  SetText(data.tbl_supply_row, " %5u(%3d%%)  %5u(%3d%%)  %5u(%3d%%)   %2d/%2d ");
  SetText(data.tbl_supply_blank, "     ----         ----         ----     ------ ");
  data.weapons_dialog = { 0x000e, 0x0080, 0x0045, 0x0119, 0x599d, 0x9f, ListDraw::TYPES, ListSelect::NONE };
  data.city_widths    = { 0x2c, 0x2d, 0x2d, 0x2d, 0x28 };
  SetText(data.tbl_str_header, "  Str   Dmg ");
  SetText(data.tbl_str_name, " %-12s ");
  SetText(data.tbl_str_row, " %4d  %4d ");
  data.city_dialog  = { 0x0009, 0x0080, 0x0046, 0x0119, 0x5a51, 0x16, ListDraw::SUPPLIES, ListSelect::NONE };
  data.stock_widths = { 0x2e, 0x26, 0x26 };
  data.save_widths  = { 0x27, 0x2a, 0x28, 0x2f, 0x2a, 0x2a };
  data.stock_dialog = { 0x001a, 0x0080, 0x0036, 0x0119, 0x5a8b, 0x07, ListDraw::STRENGTH, ListSelect::NONE };
  data.save_list    = { 0x000a, 0x0018, 0x0048, 0x00d3, 0x5a8f, 0x15, ListDraw::SAVES, ListSelect::SAVE_ACCEPT };
  SetText(data.tbl_player_computer, "Computer");
  SetText(data.tbl_player_user, "  User  ");
  SetText(data.tbl_save_header, " Scenario  Dfclty  Date(1939-45)    Axis     Allies  ");
  SetText(data.tbl_save_number, "No.%2d");
  SetText(data.tbl_map_france, "France");
  SetText(data.tbl_map_africa, "Africa");
  SetText(data.tbl_map_kursk, "Kursk ");
  SetText(data.tbl_map_dday, "D-Day ");
  SetText(data.tbl_map_bulge, "Bulge ");
  SetText(data.tbl_map_berlin, "Berlin");
  SetText(data.tbl_level_veasy, "V.Easy");
  SetText(data.tbl_level_easy, "Easy  ");
  SetText(data.tbl_level_avg, "Avg   ");
  SetText(data.tbl_level_hard, "Hard  ");
  SetText(data.tbl_level_vhard, "V.Hard");
  SetText(data.tbl_save_row, "  %6s   %5s  %2d/%2d/4%d %2dH   %8s  %8s ");
  data.tbl_player_names[0] = 0x5abd;
  data.tbl_player_names[1] = 0x5ac6;
  data.music_current       = 0xff;
  data.effect_current      = 0xff;
}
} // namespace opeu::game::detail::dglabels
