// The new-game setup: the title screen, the scenario choice over the SCESEL.DAT pictures, the players, the viewed
// or commanded side, the map type, battle viewing and difficulty, then the "ready for battle" confirmation. Returns 1
// for a new game and 0 when a saved game was loaded instead.
#include "core/scensel.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/dos-file.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/scenui.hpp"
#include "core/setmark.hpp"
#include "core/staffpnl.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "data/dgnames.hpp"
#include "io/keyboard.hpp"
#include "io/loadgame.hpp"
#include "io/loadscen.hpp"
#include "io/resopen.hpp"
#include "io/scenres.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/genconf.hpp"
#include <array>
#include <tuple>

using namespace opeu::game;

namespace {

// The scenario text buffer (1600 bytes and the bytes the routine keeps below it).
constexpr uint16_t TEXT_BYTES{ 0x648 };

} // namespace

// The text buffer the scenario text and the setting lines are loaded and formatted into, in the routine's reserve.
int opeu_new_game_setup(Game& game) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(game) };
  uint16_t const      buf{ locals.At(0) };
  auto const          at{ [&](auto const& field) -> uint16_t { return Near(data, field); } };
  auto&               side_control{ data.side_control };
  auto&               side_status{ data.side_status };
  int16_t             handle;
  uint8_t             selection;
  uint8_t             scenario;
  int                 new_game;

  opeu_load_message_graphics(game);
  handle = static_cast<uint16_t>(opeu_open_resource(game, at(data.scensel_file)));
restart:
  opeu_set_turn_context(game, 0);
  opeu_music_stop(game);
  opeu_set_turn_context(game, 7);
  opeu_finish_side_initialization(game, 1);
title:
  game.Machine().Display().Clear();
  opeu_picture_file_masked(game, 0, 0, handle, 0);
  opeu_palette_title_screen(game);
  opeu_ui_text(game, at(data.scensel_colors));
  opeu_army_position(game, 4, 0xf4);
  selection = opeu_prompt_value_required(game, at(data.scensel_start_prompt), 2, at(data.scensel_start_menu));
  if (selection == 1) {
    new_game = 1;
  scenario_menu:
    opeu_scenario_clear_panel(game);
    opeu_army_position(game, 4, 0x12c);
    selection = opeu_prompt_value_cancel(game, at(data.scensel_scenario_prompt), 6, at(data.scensel_scenario_menu));
    if (selection == 0xff) goto title;
    scenario = selection;
    opeu_load_scenario_text(game, scenario, buf);
    opeu_picture_file_masked(game, 0x15c, 0x10, handle, data.scensel_pictures[scenario]);
    opeu_army_position(game, 4, 0x18);
    opeu_scenario_clear_panel(game);
    opeu_scenario_description(game, buf);
    opeu_setup_marker_show(game, scenario);
    if (!opeu_prompt_yes_no(game, at(data.scensel_empty))) goto rejected;
    // Files shorter than the records leave the data segment as it was; the setup goes on with it.
    std::ignore = LoadScenarioFromFiles(game, static_cast<Scenario>(scenario));
    opeu_set_turn_context(game, 0);
    opeu_music_stop(game);
    opeu_set_turn_context(game, data.scenario + 1);
  players:
    opeu_scenario_clear_panel(game);
    opeu_scenario_frame(game);
    opeu_scenario_clear_text(game);
    opeu_army_position(game, 6, 7);
    switch (data.scenario + 1) {
    case 1:  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ at(data.scensel_france) }); break;
    case 2:  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ at(data.scensel_africa) }); break;
    case 3:  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ at(data.scensel_kursk) }); break;
    case 4:  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ at(data.scensel_dday) }); break;
    case 5:  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ at(data.scensel_bulge) }); break;
    default: opeu_cursor_print(game, std::array<std::uint16_t, 1>{ at(data.scensel_berlin) }); break;
    }
    opeu_army_position(game, 4, 0xf4);
    selection = opeu_prompt_value_cancel(game, at(data.scensel_players_prompt), 3, at(data.scensel_players_menu));
    if (selection == 0xff) {
      opeu_set_turn_context(game, 0);
      opeu_music_stop(game);
      opeu_set_turn_context(game, 7);
      opeu_finish_side_initialization(game, 1);
    rejected:
      opeu_setup_marker_hide(game, scenario);
      goto scenario_menu;
    }
    switch (selection) {
    case 1:
      opeu_army_position(game, 4, 0x19);
      opeu_scenario_message(game, at(data.scensel_demo));
      side_control[1] = side_control[0] = 0;
    view_side:
      opeu_scenario_clear_text(game);
      opeu_army_position(game, 4, 0xdc);
      selection = opeu_prompt_value_cancel(game, at(data.scensel_view_prompt), 2, at(data.scensel_side_menu));
      if (selection == 0xff) goto players;
      if (!data.input_mode) selection = selection == 1 ? 2 : 1;
      if (selection == 1) {
        side_status[0] = 1;
        side_status[1] = 0;
      } else {
        side_status[0] = 0;
        side_status[1] = 1;
      }
      opeu_request_format(game, std::array<std::uint16_t, 3>{ buf, at(data.scensel_view_format), data.ui_side_names[selection - 1] });
      opeu_army_position(game, 4, 0x28);
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ at(data.scensel_view_line), buf });
      data.message_delay = 7;
      break;
    case 2:
      opeu_army_position(game, 4, 0x19);
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ at(data.scensel_one_player), buf });
    command_side:
      opeu_scenario_clear_text(game);
      opeu_army_position(game, 4, 0xdc);
      selection = opeu_prompt_value_cancel(game, at(data.scensel_command_prompt), 2, at(data.scensel_side_menu));
      if (selection == 0xff) goto players;
      if (!data.input_mode) selection--;
      if (selection == 1) {
        side_status[0] = side_control[0] = 1;
        side_status[1] = side_control[1] = 0;
      } else {
        selection      = 2;
        side_status[0] = side_control[0] = 0;
        side_status[1] = side_control[1] = 1;
      }
      opeu_request_format(game, std::array<std::uint16_t, 3>{ buf, at(data.scensel_command_format), data.ui_side_names[selection - 1] });
      opeu_army_position(game, 4, 0x28);
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ at(data.scensel_command_line), buf });
      break;
    case 3:
      opeu_army_position(game, 4, 0x19);
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ at(data.scensel_two_players), buf });
      side_status[1] = side_status[0] = side_control[1] = side_control[0] = 1;
      break;
    default: goto title;
    }
  map:
    opeu_scenario_clear_text(game);
    opeu_army_position(game, 4, 0xdc);
    data.map_mode[0] = opeu_prompt_value_cancel(game, at(data.scensel_map_prompt), 2, at(data.scensel_map_menu));
    if (data.map_mode[0] == 0xff) {
      if (side_control[0] == SIDE_PLAYED && side_control[1] == SIDE_PLAYED) goto players;
      if (side_control[0] == SIDE_COMPUTER && side_control[1] == SIDE_COMPUTER) goto view_side;
      goto command_side;
    }
    data.map_mode[0]--;
    if (!data.input_mode) {
      if (!data.map_mode[0])
        data.map_mode[0] = 1;
      else
        data.map_mode[0] = 0;
    }
    opeu_request_format(game, std::array<std::uint16_t, 3>{ buf, at(data.scensel_map_format), (!data.map_mode[0] ? at(data.scensel_aerial) : at(data.scensel_field)) });
    opeu_army_position(game, 4, 0x37);
    opeu_cursor_print(game, std::array<std::uint16_t, 2>{ at(data.scensel_map_line), buf });
  battles:
    opeu_scenario_clear_text(game);
    if (side_control[0] == SIDE_COMPUTER && side_control[1] == SIDE_COMPUTER) {
      opeu_army_position(game, 4, 0xdc);
      selection = opeu_prompt_value_cancel(game, at(data.scensel_battles_prompt), 2, at(data.scensel_battles_menu));
    } else {
      opeu_army_position(game, 4, 0xf4);
      selection = opeu_prompt_value_cancel(game, at(data.scensel_battles_prompt2), 3, at(data.scensel_battles_menu2));
    }
    if (selection == 0xff) goto map;
    switch (selection) {
    case 1:
      data.battle_view = 1;
      opeu_string_copy(game, buf, at(data.scensel_view_all));
      break;
    case 2:
      data.battle_view = 0;
      opeu_string_copy(game, buf, at(data.scensel_view_none));
      break;
    default:
      data.battle_view = 0xff;
      opeu_string_copy(game, buf, at(data.scensel_view_selected));
      break;
    }
    opeu_army_position(game, 4, 0x46);
    opeu_cursor_print(game, std::array<std::uint16_t, 2>{ at(data.scensel_battles_line), buf });
  difficulty:
    if ((side_control[0] == SIDE_PLAYED && side_control[1] == SIDE_COMPUTER) || (side_control[0] == SIDE_COMPUTER && side_control[1] == SIDE_PLAYED)) {
      opeu_scenario_clear_text(game);
      opeu_army_position(game, 4, 0x124);
      data.delivery_level = opeu_prompt_value_cancel(game, at(data.scensel_difficulty_prompt), 5, at(data.scensel_difficulty_menu));
      if (data.delivery_level == 0xff) goto battles;
      data.delivery_level--;
      opeu_string_copy(game, buf, at(data.scensel_difficulty));
      switch (data.delivery_level) {
      case 0: opeu_string_append(game, buf, at(data.scensel_very_easy)); break;
      case 1: opeu_string_append(game, buf, at(data.scensel_easy)); break;
      case 2: opeu_string_append(game, buf, at(data.scensel_average)); break;
      case 3: opeu_string_append(game, buf, at(data.scensel_hard)); break;
      case 4: opeu_string_append(game, buf, at(data.scensel_very_hard)); break;
      }
      opeu_army_position(game, 4, 0x55);
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ at(data.scensel_difficulty_line), buf });
    }
    game.Machine().Display().Box({ 4, 0x86, 0x22, 0xe0 }, 7);
    opeu_army_position(game, 4, 0x154);
    if (!opeu_prompt_yes_no(game, at(data.scensel_settings_ok))) {
      opeu_army_position(game, 4, 0x154);
      opeu_cursor_print(game, std::array<std::uint16_t, 1>{ at(data.scensel_reselect) });
      game.Machine().Display().Box({ 4, 0x86, 0x22, 0xe0 }, 7);
      if ((side_control[0] == SIDE_PLAYED && side_control[1] == SIDE_COMPUTER) || (side_control[0] == SIDE_COMPUTER && side_control[1] == SIDE_PLAYED)) goto difficulty;
      goto battles;
    }
    if (side_control[0] == SIDE_COMPUTER && side_control[1] == SIDE_COMPUTER) {
      opeu_confirm_generals(game);
      game.Machine().Display().Box({ 2, 0x86, 0x22, 0x154 }, 7);
      opeu_army_position(game, 4, 0x154);
      if (opeu_prompt_yes_no(game, at(data.scensel_ready_demo))) goto start;
      goto restart;
    }
    opeu_delay_units(game, 15);
    opeu_ui_text(game, at(data.scensel_ready_colors));
    opeu_confirm_generals(game);
    opeu_army_position(game, 0x14, 0x170);
  } else if (selection == 2) {
    new_game = 0;
    opeu_ui_text(game, at(data.scensel_load_colors));
    if (!opeu_load_saved_game(game)) goto restart;
    opeu_army_position(game, 0x1c, 0xbc);
  } else
    goto title;
  opeu_staff_restore_bottom(game);
  opeu_text_paper_set(game, 4);
  opeu_ui_text(game, opeu_title_box(game, 0x1c, 0xbc, at(data.scensel_ready_blank)));
  opeu_army_position(game, 0x1c, 0xbc);
  if (!opeu_prompt_yes_no(game, at(data.scensel_ready))) goto restart;
start:
  opeu_load_scenario_graphics(game);
  opeu_ui_text(game, at(data.scensel_done_colors));
  game.Machine().Files().Close(HandleOf(handle));
  return new_game;
}
