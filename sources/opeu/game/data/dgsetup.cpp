// Data segment DS:44ec..5176 (3211 bytes). Setup-screen data: far callbacks, save/load prompts, general selection,
// ability editing, game-option menus and scenario titles. DS:5177..5183 belongs to the calendar module.
#include "data/dgsetup.hpp"
#include "data/image.hpp"

#include <array>
#include <cstddef>

namespace opeu::game::detail::dgsetup {

auto InitialSetupText(DataSegment& data) -> void {
  // clang-format off
  data.glyph_draw = { {
    { 0x28a8, 0x0000 },
    { 0x28c5, 0x0000 },
  } };
  // clang-format on
  data.text_erase = { 0x08, 0x1b, 0x4d, 0x30, 0x20, 0x08 };
  SetText(data.prompt_format, "%s");
  SetText(data.question_format, "%s");
  SetText(data.menu_format, "\033C%c\033B0\033M4%s");
  SetText(data.numdlg_format, "\033C7\033B1%2d");
  SetText(data.ems_device_name, "EMMXXXX0");
  SetEach(std::to_array<std::uint16_t>({ 0x0033, 0x00d8, 0x0033, 0x00d8, 0x003f, 0x0168, 0x0043, 0x00ca, 0x0032, 0x00e0, 0x0033, 0x00de, 0x0039, 0x00ce }),
          [&](std::size_t index, std::uint16_t value) { data.setup_marker_points[index] = value; });
  SetEach(std::to_array<std::uint16_t>({ 0x000d, 0x0012, 0x0017, 0x001c, 0x0021, 0x0026 }), [&](std::size_t index, std::uint16_t value) { data.mgamen_blocks[index] = value; });
  SetText(data.savedata_name, "SAVEDATA");
  SetText(data.no_saved_games_text, "Saved games do not exist.");
  SetText(data.load_print_format, "%s");
  SetText(data.load_savedata_name, "SAVEDATA");
  SetText(data.load_which_text, "Load which game?");
  SetText(data.loading_text, "Loading saved game...");
  SetText(data.loaded_text, "Saved game has been loaded.");
  SetText(data.load_failed_text, "Cannot find that saved game.");
  SetText(data.play_widths, "-&&&&&&");
  SetText(data.reinforce_widths, "-&&&&&&&");
  data.play_dialog = {
    0x0012, 0x00e8, 0x004b, 0x015f, 0x45ef, 0x15, { 0x121e, 0x06db },
          { 0x0f3e, 0x06db }
  };
  data.reinforce_dialog = {
    0x0012, 0x00e8, 0x004b, 0x015f, 0x45f7, 0x15, { 0x121e, 0x06db },
          { 0x0d28, 0x06db }
  };
  SetText(data.division_reserve_mark, "\033C6(R)\033C7");
  SetText(data.division_strength_format, "Div %d%s\012\012Inf%5d\012Tnk%5d\012Aty%5d");
  SetText(data.chief_ops_format, "\033C%d%-10.10s\033C7");
  SetText(data.chief_ops_prompt, "Who will be your Chief of Operations?");
  SetText(data.chief_intel_format, "\033C%d%-10.10s\033C7");
  SetText(data.chief_intel_prompt, "Who will be you Chief of Intelligence?");
  SetText(data.division_format, "\033C%d%-10.10s\033C7");
  SetText(data.division_short_format, "\033C%d%-7.7s\033C7");
  SetText(data.division_long_format, "\033C%d%-10.10s\033C7");
  SetText(data.role_prompt, "Who will be %s?");
  SetText(data.division_prompt, "Who will command Division %d?");
  SetText(data.general_header, "Lead  Chrm  Brvy  Hlth  Infy  Arty  Tank");
  SetText(data.general_name_format, "%-12.12s");
  SetText(data.general_stats_format, "%2d    %2d    %2d    %2d    %2d    %2d    %2d");
  SetText(data.text_477c, "The Axis Army");
  SetText(data.text_478a, "The Allied Army");
  SetText(data.side_title_format, "\033Y1%s\033Y0");
  data.side_titles[0]  = 0x477c;
  data.side_titles[1]  = 0x478a;
  data.reinforce_title = { 0x1b, 0x43, 0x36, 0x28, 0x52, 0x29, 0x1b, 0x43, 0x37, 0x65, 0x69, 0x6e, 0x66, 0x6f, 0x72, 0x63, 0x65, 0x6d, 0x65, 0x6e, 0x74, 0x73 };
  SetText(data.reinforce_mouse_help, "Use the \033C6right-click\033C7 to change abilities. "
                                     "\033C6Left-click\033C7 after making all changes.");
  SetText(data.reinforce_key_help, "Use the\033C6 ESC/0 key\033C7 to change abilities. \033C6Hit "
                                   "Enter\033C7 after making all changes.");
  SetText(data.play_prompt, "Who will you play ?");
  SetText(data.use_generals_prompt, "Use these generals?");
  SetText(data.scenario_message_format, "\033C1\033B7%s\012\033C0\033B7");
  SetText(data.text_48aa, " New Game");
  SetText(data.text_48b4, " Load Game ");
  SetText(data.text_48c0, " Demo Mode");
  SetText(data.text_48cb, " One Player");
  SetText(data.text_48d7, " Two Players");
  SetText(data.text_48e4, " The Allies");
  SetText(data.text_48f0, " The Axis");
  SetText(data.text_48fa, " Field Map");
  SetText(data.text_4905, " Aerial Map");
  SetText(data.text_4911, " View Battles ");
  SetText(data.text_4920, " Don't View ");
  SetText(data.text_492d, " View");
  SetText(data.text_4933, " Don't View");
  SetText(data.text_493f, " Choose at Battle ");
  SetText(data.text_4952, " Very Easy");
  SetText(data.text_495d, " Easy");
  SetText(data.text_4963, " Average");
  SetText(data.text_496c, " Hard");
  SetText(data.text_4972, " Very Hard");
  SetText(data.text_497d, " Invasion of France");
  SetText(data.text_4991, " The Battle for North Africa");
  SetText(data.text_49ae, " The Armored Battle at Kursk");
  SetText(data.text_49cb, " D-Day at Normandy");
  SetText(data.text_49de, " The Battle of the Bulge");
  SetText(data.text_49f7, " The Battle for Berlin");
  SetText(data.scensel_colors, "\033C0\033B7\033R$");
  SetText(data.scensel_france, " Invasion of France ");
  SetText(data.scensel_africa, " Battle for North Africa ");
  SetText(data.scensel_kursk, " Kursk Offensive ");
  SetText(data.scensel_dday, " D-Day Invasion ");
  SetText(data.scensel_bulge, " Battle of the Bulge ");
  SetText(data.scensel_berlin, " Fall of Berlin ");
  SetText(data.scensel_demo, " * Demo Play    ");
  SetText(data.scensel_view_format, " * View %s forces  ");
  SetText(data.scensel_view_line, "\033C1\033B7%s\012\033C0\033B7");
  SetText(data.scensel_one_player, "\033C1\033B7 * One Player  \012\033C0\033B7");
  SetText(data.scensel_command_format, "Command %s forces  ");
  SetText(data.scensel_command_line, "\033C1\033B7 * %s\012\033C0\033B7");
  SetText(data.scensel_two_players, "\033C1\033B7 * Two Players  \012\033C0\033B7");
  SetText(data.scensel_aerial, " Aerial");
  SetText(data.scensel_field, " Field ");
  SetText(data.scensel_map_format, "Map type: %s        ");
  SetText(data.scensel_map_line, "\033C1\033B7 * %s\012\033C0\033B7");
  SetText(data.scensel_view_all, "View All Battles      ");
  SetText(data.scensel_view_none, "Don't View Battles    ");
  SetText(data.scensel_view_selected, "Watch Selected Battles");
  SetText(data.scensel_battles_line, "\033C1\033B7 * %s\012\033C0\033B7");
  SetText(data.scensel_difficulty, "Difficulty set to ");
  SetText(data.scensel_very_easy, "Very Easy");
  SetText(data.scensel_easy, "Easy     ");
  SetText(data.scensel_average, "Average  ");
  SetText(data.scensel_hard, "Hard     ");
  SetText(data.scensel_very_hard, "Very Hard");
  SetText(data.scensel_difficulty_line, "\033C1\033B7 * %s\012\033C0\033B7");
  SetText(data.scensel_settings_ok, "Are the settings OK?");
  SetText(data.scensel_reselect, "Please reselect your settings.\033W2");
  SetText(data.scensel_ready_demo, "\033C0\033B7\033R$Are you ready for battle?");
  SetText(data.scensel_ready_colors, "\033C7\033B0");
  SetText(data.scensel_load_colors, "\033C7\033B0");
  SetText(data.scensel_ready_blank, "                            ");
  SetText(data.scensel_ready, " Are you ready for battle?");
  SetText(data.scensel_done_colors, "\033M0\033C7\033B0");
  // clang-format off
  data.scensel_start_menu = { {
    { 0x0005, 0x00a0, 0x000e, 0x0700, 0x48aa, 0x0001 }, // " New Game"
    { 0x0005, 0x00b8, 0x000e, 0x0700, 0x48b4, 0x0002 }, // " Load Game "
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.scensel_start_prompt, "Please select one of the above.");
  // clang-format off
  data.scensel_players_menu = { {
    { 0x0005, 0x00a0, 0x0013, 0x0700, 0x48c0, 0x0001 }, // " Demo Mode"
    { 0x0005, 0x00b8, 0x0013, 0x0700, 0x48cb, 0x0002 }, // " One Player"
    { 0x0005, 0x00d0, 0x0013, 0x0700, 0x48d7, 0x0003 }, // " Two Players"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.scensel_players_prompt, "\033C0\033B7How many players?");
  SetText(data.scensel_view_prompt, "\033C0\033B7Which side will you view?");
  // clang-format off
  data.scensel_side_menu = { {
    { 0x0005, 0x00a0, 0x000d, 0x0700, 0x48e4, 0x0002 }, // " The Allies"
    { 0x0005, 0x00b8, 0x000d, 0x0700, 0x48f0, 0x0001 }, // " The Axis"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.scensel_command_prompt, "\033C0\033B7Which side will you command?");
  // clang-format off
  data.scensel_map_menu = { {
    { 0x0005, 0x00a0, 0x000f, 0x0700, 0x48fa, 0x0002 }, // " Field Map"
    { 0x0005, 0x00b8, 0x000f, 0x0700, 0x4905, 0x0001 }, // " Aerial Map"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.scensel_map_prompt, "\033C0\033B7Which map will you use?");
  // clang-format off
  data.scensel_battles_menu = { {
    { 0x0005, 0x00a0, 0x000e, 0x0700, 0x4911, 0x0001 }, // " View Battles "
    { 0x0005, 0x00b8, 0x000e, 0x0700, 0x4920, 0x0002 }, // " Don't View "
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.scensel_battles_prompt, "\033C0\033B7View Battles? ");
  // clang-format off
  data.scensel_battles_menu2 = { {
    { 0x0005, 0x00a0, 0x0012, 0x0700, 0x492d, 0x0001 }, // " View"
    { 0x0005, 0x00b8, 0x0012, 0x0700, 0x4933, 0x0002 }, // " Don't View"
    { 0x0005, 0x00d0, 0x0012, 0x0700, 0x493f, 0x0003 }, // " Choose at Battle "
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.scensel_battles_prompt2, "\033C0\033B7View Battles? ");
  // clang-format off
  data.scensel_difficulty_menu = { {
    { 0x0005, 0x00a0, 0x000b, 0x0700, 0x4952, 0x0001 }, // " Very Easy"
    { 0x0005, 0x00b8, 0x000b, 0x0700, 0x495d, 0x0002 }, // " Easy"
    { 0x0005, 0x00d0, 0x000b, 0x0700, 0x4963, 0x0003 }, // " Average"
    { 0x0005, 0x00e8, 0x000b, 0x0700, 0x496c, 0x0004 }, // " Hard"
    { 0x0005, 0x0100, 0x000b, 0x0700, 0x4972, 0x0005 }, // " Very Hard"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.scensel_difficulty_prompt, "\033C0\033B7Select Difficulty Level");
  // clang-format off
  data.scensel_scenario_menu = { {
    { 0x0005, 0x0090, 0x001e, 0x0700, 0x497d, 0x0001 }, // " Invasion of France"
    { 0x0005, 0x00a8, 0x001e, 0x0700, 0x4991, 0x0002 }, // " The Battle for North Africa"
    { 0x0005, 0x00c0, 0x001e, 0x0700, 0x49ae, 0x0003 }, // " The Armored Battle at Kursk"
    { 0x0005, 0x00d8, 0x001e, 0x0700, 0x49cb, 0x0004 }, // " D-Day at Normandy"
    { 0x0005, 0x00f0, 0x001e, 0x0700, 0x49de, 0x0005 }, // " The Battle of the Bulge"
    { 0x0005, 0x0108, 0x001e, 0x0700, 0x49f7, 0x0006 }, // " The Battle for Berlin"
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, // the end
  } };
  // clang-format on
  SetText(data.scensel_scenario_prompt, "\033C0\033B7Which scenario will you play?");
  SetEach(std::to_array<std::uint16_t>({ 0x001d, 0x001d, 0x002c, 0x0033, 0x0041, 0x004c, 0x005b }), [&](std::size_t index, std::uint16_t value) { data.scensel_pictures[index] = value; });
  SetText(data.memory_message, "\033C7\033B0\033=  You do not have enough "
                               "memory free to run this program.\012");
  SetText(data.memory_advice, "Please refer to the section FINDING FREE MEMORY "
                              "in the Reference Card.\012");
  SetText(data.memory_exit, "Hit any key to exit to DOS.");
  SetText(data.setup_name_bmapseg, "bmapseg");
  SetText(data.setup_name_hunitseg, "hunitseg");
  SetText(data.setup_name_grpseg, "grpseg");
  SetText(data.setup_name_ms_winseg, "ms_winseg");
  SetText(data.setup_name_vseg, "vseg");
  SetText(data.setup_name_hdispseg, "hdispseg");
  SetText(data.setup_name_armsseg, "armsseg");
  SetText(data.setup_name_kaoseg, "kaoseg");
  SetText(data.setup_name_mdispseg, "mdispseg");
  SetText(data.setup_name_ms_seg, "ms_seg");
  SetText(data.setup_name_hfontseg, "hfontseg");
  SetText(data.setup_name_mmapseg, "mmapseg");
  data.end_palette       = { 0x00, 0x00, 0x00, 0x09, 0x01, 0x04, 0x03, 0x0d, 0x03, 0x0b, 0x0d, 0x07, 0x04, 0x04, 0x09, 0x0b, 0x07, 0x0b, 0x06, 0x0f, 0x0c, 0x0f, 0x0f, 0x0f,
                             0x02, 0x06, 0x04, 0x05, 0x09, 0x07, 0x07, 0x0c, 0x0a, 0x04, 0x06, 0x06, 0x07, 0x09, 0x09, 0x08, 0x0a, 0x0a, 0x02, 0x01, 0x05, 0x09, 0x06, 0x09 };
  data.end_palette_final = { 0x00, 0x00, 0x00, 0x0f, 0x05, 0x03, 0x05, 0x0f, 0x05, 0x09, 0x0f, 0x0d, 0x07, 0x07, 0x0b, 0x0f, 0x09, 0x0b, 0x05, 0x05, 0x07,
                             0x0f, 0x0f, 0x0f, 0x07, 0x07, 0x07, 0x0a, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x08, 0x0a, 0x00, 0x0a };
  data.text_50c1         = { 0x0a, 0x0a, 0x0a, 0x0a, 0x0a };
  data.end_text_format   = { 0x1b, 0x42, 0x30, 0x1b, 0x52, 0x29, 0x25, 0x73, 0x1b, 0x52, 0x2c };
  SetText(data.end_picture_name, "endgrp.npk");
  SetText(data.end_salute, "I salute you Commander %s!");
  // clang-format off
  data.end_victory_texts = { {
    { 0x0000, 0x384f },
    { 0x0049, 0x384f },
    { 0x00c6, 0x384f },
    { 0x0143, 0x384f },
  } };
  // clang-format on
  // clang-format off
  data.end_defeat_texts = { {
    { 0x01bc, 0x384f },
    { 0x01ea, 0x384f },
    { 0x021a, 0x384f },
    { 0x021a, 0x384f },
  } };
  // clang-format on
  SetText(data.name_bmapseg, "bmapseg");
  SetText(data.name_mmapseg, "mmapseg");
  SetText(data.name_hdispseg, "hdispseg");
  SetText(data.name_hunitseg, "hunitseg");
  SetText(data.name_grpseg, "grpseg");
  SetText(data.name_ms_winseg, "ms_winseg");
  SetText(data.name_ms_seg, "ms_seg");
  SetText(data.name_hfontseg, "hfontseg");
  SetText(data.name_vseg, "vseg");
  SetText(data.name_kaoseg, "kaoseg");
  SetText(data.name_armsseg, "armsseg");
  data.name_mdispseg = { 0x6d, 0x64, 0x69, 0x73, 0x70, 0x73, 0x65, 0x67 };
}
} // namespace opeu::game::detail::dgsetup
