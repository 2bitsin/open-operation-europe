// Data segment DS:9e10..ae2d (4126 bytes). Request, air-force and special-operations messages with menu-item tables.
#include "data/dgair.hpp"
#include "data/image.hpp"

#include <array>
#include <cstddef>

namespace opeu::game::detail::dgair {

auto InitialAirText(DataSegment& data) -> void {
  SetText(data.rq_header_text,
          "\033B1\033C7Request   Dest     Type      Wait\033B0");
  SetText(data.text_9e3b, "Infantry");
  SetText(data.text_9e44, "Mechanized");
  SetText(data.text_9e4f, "Armor");
  SetText(data.text_9e55, "Artillery");
  SetText(data.text_9e5f, "Engineer");
  SetText(data.text_9e68, "Supply");
  SetText(data.text_9e6f, "Supplies");
  SetText(data.text_9e78, "Battalions");
  SetText(data.text_9e83, "Regiments");
  SetText(data.rq_title_text, "Request");
  SetText(data.rq_supply_row_text,
          "\033B1Supplies  \033C6%-8.8s --------   %2d\033C7\033B0");
  SetText(data.rq_supply_filed_text,
          "\033B1Supplies  \033C6%-8.8s --------    ??\033C7\033B0");
  SetText(data.rq_battalion_row_text,
          "\033B1Battalion \033C6%-8.8s %-10.10s  %2d\033C7\033B0");
  SetText(data.rq_battalion_filed_text,
          "\033B1Battalion \033C6%-8.8s %-10.10s  ??\033C7\033B0");
  SetText(data.rq_regiment_row_text,
          "\033B1Regiment  \033C6%--------  %-10.10s  %2d\033C7\033B0");
  SetText(data.rq_regiment_filed_text,
          "\033B1Regiment  \033C6%--------  %-10.10s  ??\033C7\033B0");
  SetText(data.rq_menu_prompt_text, "Your request, sir");
  SetText(data.rq_busy_text, "That is already under\012preparation. You "
                             "cannot\012change it now.\033S");
  SetText(data.rq_no_city_text, "We have no friendly cities!\033E2");
  SetEach(std::to_array<std::uint16_t>(
              { 0x9e3b, 0x9e44, 0x9e4f, 0x9e55, 0x9e5f, 0x9e68 }),
          [&](std::size_t index, std::uint16_t value) {
            data.rq_regiment_kind_names[index] = value;
          });
  data.request_handlers = {
    { { 0x0136, 0x22f9 }, { 0x03b6, 0x22f9 }, { 0x0780, 0x22f9 } }
  };
  SetEach(std::to_array<std::uint16_t>({ 0x0034, 0x0018, 0x000b, 0x0700 }),
          [&](std::size_t index, std::uint16_t value) {
            data.rq_menu_items[index] = value;
          });
  data.near_a032  = 0x9e6f;
  data.bytes_a034 = { { 0x01, 0x80, 0x34, 0x00, 0x30, 0x00, 0x0b, 0x00, 0x00,
                        0x07 } };
  data.near_a03e  = 0x9e78;
  data.bytes_a040 = { { 0x02, 0x80, 0x34, 0x00, 0x48, 0x00, 0x0b, 0x00, 0x00,
                        0x07 } };
  data.near_a04a  = 0x9e83;
  data.bytes_a04c = { { 0x03, 0x80 } };
  SetText(data.air_question_mark, "? ");
  SetText(data.air_range_format, "(%u-%u)? \033B1");
  SetText(data.air_input_begin, "\033B1");
  SetText(data.air_input_end, "\033B0");
  SetText(data.air_hostile_city, "hostile city");
  SetText(data.air_understood, "Understood, Commander.\033S");
  SetText(data.text_a098, "a carpet bombing");
  SetText(data.text_a0a9, "an attack sortie");
  SetText(data.text_a0ba, "a strafing");
  SetText(data.air_plane, "plane");
  SetText(data.air_planes, "planes");
  SetText(data.air_hour, "hour");
  SetText(data.air_hours, "hours");
  data.bombing_flights = { { { 0x0a, 0x0b }, { 0x05, 0x06 }, { 0x01, 0x05 } } };
  data.bombing_names[0] = 0xa098;
  data.bombing_names[1] = 0xa0a9;
  data.bombing_names[2] = 0xa0ba;
  data.bytes_a0ea       = { { 0x43, 0x61, 0x72, 0x70, 0x65, 0x74, 0x20, 0x42,
                              0x6f, 0x6d, 0x62, 0x69, 0x6e, 0x67, 0x20, 0x28,
                              0x31, 0x30, 0x2d, 0x32, 0x30, 0x20, 0x70, 0x6c,
                              0x61, 0x6e, 0x65, 0x73, 0x29 } };
  SetText(data.text_a108, "Attack Sortie  (5-10 planes)");
  SetText(data.text_a125, "Strafing Run   (1- 5 planes)");
  SetText(data.air_null_order, "NULL!!!\033S");
  SetText(data.bombing_title, "Bombing Attack");
  SetText(data.bombing_site_prompt, "Specify bombing site.");
  data.bombing_menu = { { 0x2e, 0x00, 0xa3, 0x00, 0x1d, 0x00, 0x00,
                          0x07, 0xea, 0xa0, 0x01, 0x00, 0x2e, 0x00,
                          0xbb, 0x00, 0x1d, 0x00, 0x00, 0x07 } };
  data.near_a186    = 0xa108;
  SetEach(
      std::to_array<std::uint16_t>({ 0x0002, 0x002e, 0x00d3, 0x001d, 0x0700 }),
      [&](std::size_t index, std::uint16_t value) {
        data.words_a188[index] = value;
      });
  data.near_a192  = 0xa125;
  data.bytes_a194 = { { 0x03 } };
  SetText(data.drop_ask_troops, "How many troops will you\012assign, sir");
  SetText(data.drop_troops_assigned, "Assign the soldiers, sir.");
  SetText(data.drop_which_regiment, "Assign paratroopers to which regiment?");
  SetText(data.drop_assign_soldiers, "Assign the soldiers, sir.");
  SetText(data.drop_command_post,
          "Choose a command post for the regiment, sir.");
  SetText(data.drop_understood, "Understood, Commander.\033S");
  SetText(data.drop_plane, "plane");
  SetText(data.drop_planes, "planes");
  SetText(data.drop_no_paratroopers, "There are no more paratroopers!\033S");
  SetText(data.drop_cancel, "Cancel airdrop?");
  SetText(data.drop_no_transports,
          "There are no transport planes available.\033E1\033S");
  SetText(data.drop_no_regiments,
          "You cannot increase your regiments.\033E1\033S");
  SetText(data.drop_no_battalions,
          "You cannot increase your battalions.\033E1\033S");
  SetText(data.drop_where, "Where should we carry out the air drop, sir?");
  SetText(data.drop_paratroopers, "Paratroopers");
  SetText(data.drop_hours_prompt, "Drop them in how many\012hours, sir");
  SetText(data.drop_ask_hq, "We will ask Air Force HQ\012for an assault "
                            "airdrop.\012Is this correct, sir?");
  SetText(data.lift_lead_format,
          "Preparations will take at least %d hours.\033S");
  SetText(data.lift_understood, "Understood, Commander.\033S");
  SetText(data.air_lift_plane, "plane");
  SetText(data.air_lift_planes, "planes");
  SetText(data.air_lift_hour, "hour");
  SetText(data.air_lift_hours, "hours");
  SetText(data.lift_start_format,
          "In %d %s, %d %s will start their cargo lift.\033S");
  SetText(data.lift_cancel, "Cancel the airlift?");
  SetText(data.lift_no_planes,
          "There are no cargo planes ready for action.\033E1");
  SetText(data.lift_ask_ammo, "Airlift how much\012ammo");
  SetText(data.lift_ask_fuel, "Airlift how much\012fuel");
  SetText(data.lift_ask_food, "Airlift how much\012food");
  SetText(data.lift_select_point, "Select cargo drop point.");
  SetText(data.lift_cargo_name, "Cargo");
  SetText(data.lift_ask_hours, "Drop in how many\012hours");
  SetText(
      data.lift_confirm,
      "We will ask Air Force HQ for a cargo lift.\012Is this correct, sir?");
  SetText(data.air_header_format, "\033B1Target     Planes Wait\033B0");
  SetText(data.text_a56f, "Air Strike");
  SetText(data.text_a57a, "Paratroopers");
  SetText(data.text_a587, "Cargo Drop");
  SetText(data.air_title, "Air Force");
  SetText(data.air_strike_format,
          "\033B1Air Strike\033C6 %-10.10s %4u   %2d\033C7\033B0");
  SetText(data.air_drop_format,
          "\033B1Paratroops\033C6 %-10.10s %4u   %2d\033C7\033B0");
  SetText(data.air_lift_format,
          "\033B1Cargo Drop\033C6 %-10.10s %4u   %2d\033C7\033B0");
  SetText(data.air_mission_question, "Which air mission, sir");
  data.air_missions = {
    { { 0x05b0, 0x23b2 }, { 0x0b28, 0x23b2 }, { 0x0e20, 0x23b2 } }
  };
  SetEach(std::to_array<std::uint16_t>({ 0x003a, 0x0018, 0x000c, 0x0700 }),
          [&](std::size_t index, std::uint16_t value) {
            data.air_mission_menu[index] = value;
          });
  data.near_a646  = 0xa56f;
  data.bytes_a648 = { { 0x01, 0x80, 0x3a, 0x00, 0x30, 0x00, 0x0c, 0x00, 0x00,
                        0x07 } };
  data.near_a652  = 0xa57a;
  data.bytes_a654 = { { 0x02, 0x80, 0x3a, 0x00, 0x48, 0x00, 0x0c, 0x00, 0x00,
                        0x07 } };
  data.near_a65e  = 0xa587;
  data.bytes_a660 = { { 0x03, 0x80 } };
  SetText(data.raid_title, "Raid");
  SetText(data.raid_prompt, "Attack which unit?");
  SetText(data.raid_city_question, "The %s\012garrison?");
  SetText(data.raid_unit_question, "%s, sir?");
  SetText(data.raid_refusal_1, "I think that attack\012is too reckless.\033S");
  SetText(data.raid_refusal_2,
          "My men will be slaughtered in an\012attack like that!\033S");
  SetText(data.raid_refusal_3,
          "I won't leave my men\012to a man who lost his mind!\033S");
  SetText(data.raid_refusal_4, "Commando raids are\012not that easy.\033S");
  SetText(data.raid_lieutenant, "Lt. %s\012will lead the mission.\012OK? ");
  SetText(data.sabotage_title, "Sabotage");
  SetText(data.sabotage_prompt, "Which city's depot\012shall we sabotage?");
  SetText(data.sabotage_city_question, "%s,\012right sir?");
  SetText(data.sabotage_refusal_1,
          "I think that mission\012is too risky, sir.\033S");
  SetText(data.sabotage_refusal_2,
          "I can't let my men die\012on such an operation.\033S");
  SetText(data.sabotage_refusal_3,
          "My men would butchered\012on a mission like that.\033S");
  SetText(data.sabotage_refusal_4, "Sabotage is not that easy, sir.\033S");
  SetText(
      data.sabotage_duration,
      "Understood, sir.\012It will take about\012%d hours to complete.\033S");
  SetText(data.sabotage_lieutenant,
          "I'll assign Lt. %s\012to the mission.\012OK? ");
  SetText(data.cut_comm_title, "Cut Comm");
  SetText(data.cut_comm_prompt, "Cut communications\012of which unit?");
  SetText(data.cut_comm_city_question, "The %s\012garrison, right sir?");
  SetText(data.cut_comm_unit_question, "%s, sir?");
  SetText(data.cut_comm_refusal_1, "I'm afraid that plan is foolish.\033S");
  SetText(data.cut_comm_refusal_2,
          "I won't let my men die\012on such a mission!\033S");
  SetText(data.cut_comm_refusal_3,
          "I will not let you send\012my men on a death mission.\033S");
  SetText(data.cut_comm_refusal_4,
          "Now is not a good time\012to move our Special Forces.\033S");
  SetText(data.cut_comm_hour, "hour");
  SetText(data.cut_comm_hours, "hours");
  SetText(data.cut_comm_duration,
          "Yes, sir.\012The mission will take \012about %d %s.\033S");
  SetText(data.cut_comm_lieutenant,
          "Lt. %s\012will take charge of\012the mission.\012OK? ");
  SetText(data.assassinate_title, "Assasinate");
  SetText(data.assassinate_prompt, "Assasinate which commander?");
  SetText(data.assassinate_unit_question, "The Commander of enemy\012%s?");
  SetText(data.assassinate_refusal_1, "That plan is crazy!\033S");
  SetText(data.assassinate_refusal_2,
          "I will not sentence my\012men to certain death!\033S");
  SetText(data.assassinate_refusal_3, "Are you insane?!\033S");
  SetText(data.assassinate_refusal_4,
          "Assassination is only a\012short-term solution.\033S");
  SetText(data.assassinate_duration,
          "Affirmative, sir.\012The mission will take\012about %d hours.\033S");
  SetText(data.assassinate_lieutenant,
          "Lt. %s\012will lead the mission.\012OK? ");
  SetText(data.recon_title, "Recon");
  SetText(data.recon_prompt, "Recon which unit/city?");
  SetText(data.recon_city_question, "%s, sir?");
  SetText(data.recon_unit_question, "Enemy unit %s?");
  SetText(data.recon_refusal_1, "I'm afraid that plan\012is ridiculous.\033S");
  SetText(data.recon_refusal_2,
          "All my men would be\012killed on such a mission.\033S");
  SetText(data.recon_refusal_3, "We don't need info\012on that position.\033S");
  SetText(data.recon_hour, "hour");
  SetText(data.recon_hours, "hours");
  SetText(data.recon_duration,
          "Yes, sir.\012We will report\012in about %d %s.\033S");
  SetText(data.recon_lieutenant, "Lt. %s\012will lead the recon\012team. OK?");
  SetText(data.text_ac68, "Recon");
  SetText(data.text_ac6e, "Cut Lines");
  SetText(data.text_ac78, "Sabotage");
  SetText(data.text_ac81, "Raid");
  SetText(data.text_ac86, "Assassinate");
  SetText(data.text_ac92, "City Raid");
  SetText(data.text_ac9c, "Unit Raid");
  SetText(data.text_aca6, "Supply Raid");
  SetText(data.text_acb2, "Cut City Comm");
  SetText(data.text_acc0, "Cut Unit Comm");
  SetText(data.text_acce, "Eliminate Cmdr");
  SetText(data.text_acdd, "Recon City");
  SetText(data.text_ace8, "Recon Unit");
  SetText(data.special_forces_title, "Special Forces");
  SetText(data.special_orders_line, "\033B1Orders     \033C6%s\033C7\033B0");
  SetText(data.special_target_line, "\033B1Target     \033C6%s\033C7\033B0");
  SetText(data.special_time_line, "\033B1Req'd Time \033C6%d\033C7\033B0");
  SetText(data.special_cancel_question,
          "The Special Forces\012are now en route.\012Should we cancel "
          "their\012mission, sir?");
  SetText(data.special_select_question, "Which will you select,\012sir");
  data.special_missions = { { { 0x0840, 0x24d5 },
                              { 0x0414, 0x24d5 },
                              { 0x0252, 0x24d5 },
                              { 0x0008, 0x24d5 },
                              { 0x0668, 0x24d5 } } };
  SetEach(std::to_array<std::uint16_t>({ 0x0041, 0x0018, 0x000c, 0x0700 }),
          [&](std::size_t index, std::uint16_t value) {
            data.special_menu[index] = value;
          });
  data.near_add2  = 0xac68;
  data.bytes_add4 = { { 0x01, 0x80, 0x41, 0x00, 0x30, 0x00, 0x0c, 0x00, 0x00,
                        0x07 } };
  data.near_adde  = 0xac6e;
  data.bytes_ade0 = { { 0x02, 0x80, 0x41, 0x00, 0x48, 0x00, 0x0c, 0x00, 0x00,
                        0x07 } };
  data.near_adea  = 0xac78;
  data.bytes_adec = { { 0x03, 0x80, 0x41, 0x00, 0x60, 0x00, 0x0c, 0x00, 0x00,
                        0x07 } };
  data.near_adf6  = 0xac81;
  data.bytes_adf8 = { { 0x04, 0x80, 0x41, 0x00, 0x78, 0x00, 0x0c, 0x00, 0x00,
                        0x07 } };
  data.near_ae02  = 0xac86;
  data.bytes_ae04 = { { 0x05, 0x80 } };
  SetEach(std::to_array<std::uint16_t>({ 0xac92, 0xac9c, 0xaca6, 0xacb2, 0xacc0,
                                         0xacce, 0xacdd, 0xace8 }),
          [&](std::size_t index, std::uint16_t value) {
            data.special_mission_names[index] = value;
          });
}
} // namespace opeu::game::detail::dgair
