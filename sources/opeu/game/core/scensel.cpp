// The new-game setup: the title screen, the scenario choice over the SCESEL.DAT pictures, the players, the viewed
// or commanded side, the map type, battle viewing and difficulty, then the "ready for battle" confirmation. Returns 1
// for a new game and 0 when a saved game was loaded instead.
#include "core/scensel.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
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
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/genconf.hpp"

using namespace opeu::game;

namespace {

// The services the setup calls through its event; strings and buffers are near pointers.
enum Service : std::uint16_t {
  PRINT                  = 0,  // army print(format[, argument])
  FORMAT                 = 1,  // request format(destination, format, argument)
  POSITION               = 2,  // position(x, y)
  UI_TEXT                = 3,  // ui text(text)
  PROMPT_CANCEL          = 4,  // prompt with cancel(text, max, items): the item, 0xff when cancelled
  PROMPT_REQUIRED        = 5,  // prompt without cancel(text, max, items)
  YES_NO                 = 6,  // yes/no(text)
  OPEN                   = 7,  // open resource(name)
  TURN_CONTEXT           = 8,  // turn context(track)
  MUSIC_STOP             = 9,  // music stop()
  FINISH_INIT            = 10, // finish side initialization(value)
  VGA_CLEAR              = 11, // VGA clear()
  PICTURE_MASKED         = 12, // masked picture(x, y, handle, block)
  TITLE_PALETTE          = 13, // title palette()
  STRING_COPY            = 14, // string copy(destination, source)
  STRING_APPEND          = 15, // string append(destination, source)
  UI_BOX                 = 16, // ui box(x, y, width, height, style)
  DELAY                  = 17, // wait(tenths)
  TEXT_ATTRIBUTE         = 18, // text attribute(value)
  FRAMED_TEXT            = 19, // framed text(x, y, text)
  CLOSE                  = 20, // DOS close(handle)
  LOAD_SCENARIO          = 21, // load scenario(scenario)
  MARKER_SHOW            = 22, // setup marker show(scenario)
  MARKER_HIDE            = 23, // setup marker hide(scenario)
  LOAD_SCENARIO_TEXT     = 24, // load scenario text(scenario, buffer)
  LOAD_MESSAGE_GRAPHICS  = 25, // load message graphics()
  LOAD_SCENARIO_GRAPHICS = 26, // load scenario graphics()
  LOAD_SAVED_GAME        = 27, // load saved game(): nonzero when a game was loaded
  STAFF_RESTORE_BOTTOM   = 28, // staff restore bottom()
  CONFIRM_GENERALS       = 29, // confirm generals()
  SCENARIO_FRAME         = 30, // scenario frame()
  SCENARIO_MESSAGE       = 31, // scenario message(text)
  CLEAR_PANEL            = 32, // scenario clear panel()
  CLEAR_TEXT             = 33, // scenario clear text()
  SCENARIO_DESCRIPTION   = 34, // scenario description(buffer)
};

// The scenario text buffer (1600 bytes and the bytes the routine keeps below it).
constexpr uint16_t TEXT_BYTES{ 0x648 };

} // namespace

// The text buffer the scenario text and the setting lines are loaded and formatted into, in the routine's reserve.
int opeu_new_game_setup(Game& game, HostEvent event) {
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

  Call(game, event, LOAD_MESSAGE_GRAPHICS);
  handle = Call(game, event, OPEN, at(data.scensel_file));
restart:
  Call(game, event, TURN_CONTEXT, 0);
  Call(game, event, MUSIC_STOP);
  Call(game, event, TURN_CONTEXT, 7);
  Call(game, event, FINISH_INIT, 1);
title:
  Call(game, event, VGA_CLEAR);
  Call(game, event, PICTURE_MASKED, 0, 0, handle, 0);
  Call(game, event, TITLE_PALETTE);
  Call(game, event, UI_TEXT, at(data.scensel_colors));
  Call(game, event, POSITION, 4, 0xf4);
  selection = Call(game, event, PROMPT_REQUIRED, at(data.scensel_start_prompt), 2, at(data.scensel_start_menu));
  if (selection == 1) {
    new_game = 1;
  scenario_menu:
    Call(game, event, CLEAR_PANEL);
    Call(game, event, POSITION, 4, 0x12c);
    selection = Call(game, event, PROMPT_CANCEL, at(data.scensel_scenario_prompt), 6, at(data.scensel_scenario_menu));
    if (selection == 0xff) goto title;
    scenario = selection;
    Call(game, event, LOAD_SCENARIO_TEXT, scenario, buf);
    Call(game, event, PICTURE_MASKED, 0x15c, 0x10, handle, data.scensel_pictures[scenario]);
    Call(game, event, POSITION, 4, 0x18);
    Call(game, event, CLEAR_PANEL);
    Call(game, event, SCENARIO_DESCRIPTION, buf);
    Call(game, event, MARKER_SHOW, scenario);
    if (!Call(game, event, YES_NO, at(data.scensel_empty))) goto rejected;
    Call(game, event, LOAD_SCENARIO, scenario);
    Call(game, event, TURN_CONTEXT, 0);
    Call(game, event, MUSIC_STOP);
    Call(game, event, TURN_CONTEXT, static_cast<uint8_t>(data.scenario + 1));
  players:
    Call(game, event, CLEAR_PANEL);
    Call(game, event, SCENARIO_FRAME);
    Call(game, event, CLEAR_TEXT);
    Call(game, event, POSITION, 6, 7);
    switch (data.scenario + 1) {
    case 1:  Call(game, event, PRINT, at(data.scensel_france)); break;
    case 2:  Call(game, event, PRINT, at(data.scensel_africa)); break;
    case 3:  Call(game, event, PRINT, at(data.scensel_kursk)); break;
    case 4:  Call(game, event, PRINT, at(data.scensel_dday)); break;
    case 5:  Call(game, event, PRINT, at(data.scensel_bulge)); break;
    default: Call(game, event, PRINT, at(data.scensel_berlin)); break;
    }
    Call(game, event, POSITION, 4, 0xf4);
    selection = Call(game, event, PROMPT_CANCEL, at(data.scensel_players_prompt), 3, at(data.scensel_players_menu));
    if (selection == 0xff) {
      Call(game, event, TURN_CONTEXT, 0);
      Call(game, event, MUSIC_STOP);
      Call(game, event, TURN_CONTEXT, 7);
      Call(game, event, FINISH_INIT, 1);
    rejected:
      Call(game, event, MARKER_HIDE, scenario);
      goto scenario_menu;
    }
    switch (selection) {
    case 1:
      Call(game, event, POSITION, 4, 0x19);
      Call(game, event, SCENARIO_MESSAGE, at(data.scensel_demo));
      side_control[1] = side_control[0] = 0;
    view_side:
      Call(game, event, CLEAR_TEXT);
      Call(game, event, POSITION, 4, 0xdc);
      selection = Call(game, event, PROMPT_CANCEL, at(data.scensel_view_prompt), 2, at(data.scensel_side_menu));
      if (selection == 0xff) goto players;
      if (!data.input_mode) selection = selection == 1 ? 2 : 1;
      if (selection == 1) {
        side_status[0] = 1;
        side_status[1] = 0;
      } else {
        side_status[0] = 0;
        side_status[1] = 1;
      }
      Call(game, event, FORMAT, buf, at(data.scensel_view_format), data.ui_side_names[selection - 1]);
      Call(game, event, POSITION, 4, 0x28);
      Call(game, event, PRINT, at(data.scensel_view_line), buf);
      data.message_delay = 7;
      break;
    case 2:
      Call(game, event, POSITION, 4, 0x19);
      Call(game, event, PRINT, at(data.scensel_one_player), buf);
    command_side:
      Call(game, event, CLEAR_TEXT);
      Call(game, event, POSITION, 4, 0xdc);
      selection = Call(game, event, PROMPT_CANCEL, at(data.scensel_command_prompt), 2, at(data.scensel_side_menu));
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
      Call(game, event, FORMAT, buf, at(data.scensel_command_format), data.ui_side_names[selection - 1]);
      Call(game, event, POSITION, 4, 0x28);
      Call(game, event, PRINT, at(data.scensel_command_line), buf);
      break;
    case 3:
      Call(game, event, POSITION, 4, 0x19);
      Call(game, event, PRINT, at(data.scensel_two_players), buf);
      side_status[1] = side_status[0] = side_control[1] = side_control[0] = 1;
      break;
    default: goto title;
    }
  map:
    Call(game, event, CLEAR_TEXT);
    Call(game, event, POSITION, 4, 0xdc);
    data.map_mode[0] = Call(game, event, PROMPT_CANCEL, at(data.scensel_map_prompt), 2, at(data.scensel_map_menu));
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
    Call(game, event, FORMAT, buf, at(data.scensel_map_format), !data.map_mode[0] ? at(data.scensel_aerial) : at(data.scensel_field));
    Call(game, event, POSITION, 4, 0x37);
    Call(game, event, PRINT, at(data.scensel_map_line), buf);
  battles:
    Call(game, event, CLEAR_TEXT);
    if (side_control[0] == SIDE_COMPUTER && side_control[1] == SIDE_COMPUTER) {
      Call(game, event, POSITION, 4, 0xdc);
      selection = Call(game, event, PROMPT_CANCEL, at(data.scensel_battles_prompt), 2, at(data.scensel_battles_menu));
    } else {
      Call(game, event, POSITION, 4, 0xf4);
      selection = Call(game, event, PROMPT_CANCEL, at(data.scensel_battles_prompt2), 3, at(data.scensel_battles_menu2));
    }
    if (selection == 0xff) goto map;
    switch (selection) {
    case 1:
      data.battle_view = 1;
      Call(game, event, STRING_COPY, buf, at(data.scensel_view_all));
      break;
    case 2:
      data.battle_view = 0;
      Call(game, event, STRING_COPY, buf, at(data.scensel_view_none));
      break;
    default:
      data.battle_view = 0xff;
      Call(game, event, STRING_COPY, buf, at(data.scensel_view_selected));
      break;
    }
    Call(game, event, POSITION, 4, 0x46);
    Call(game, event, PRINT, at(data.scensel_battles_line), buf);
  difficulty:
    if ((side_control[0] == SIDE_PLAYED && side_control[1] == SIDE_COMPUTER) || (side_control[0] == SIDE_COMPUTER && side_control[1] == SIDE_PLAYED)) {
      Call(game, event, CLEAR_TEXT);
      Call(game, event, POSITION, 4, 0x124);
      data.delivery_level = Call(game, event, PROMPT_CANCEL, at(data.scensel_difficulty_prompt), 5, at(data.scensel_difficulty_menu));
      if (data.delivery_level == 0xff) goto battles;
      data.delivery_level--;
      Call(game, event, STRING_COPY, buf, at(data.scensel_difficulty));
      switch (data.delivery_level) {
      case 0: Call(game, event, STRING_APPEND, buf, at(data.scensel_very_easy)); break;
      case 1: Call(game, event, STRING_APPEND, buf, at(data.scensel_easy)); break;
      case 2: Call(game, event, STRING_APPEND, buf, at(data.scensel_average)); break;
      case 3: Call(game, event, STRING_APPEND, buf, at(data.scensel_hard)); break;
      case 4: Call(game, event, STRING_APPEND, buf, at(data.scensel_very_hard)); break;
      }
      Call(game, event, POSITION, 4, 0x55);
      Call(game, event, PRINT, at(data.scensel_difficulty_line), buf);
    }
    Call(game, event, UI_BOX, 4, 0x86, 0x22, 0xe0, 7);
    Call(game, event, POSITION, 4, 0x154);
    if (!Call(game, event, YES_NO, at(data.scensel_settings_ok))) {
      Call(game, event, POSITION, 4, 0x154);
      Call(game, event, PRINT, at(data.scensel_reselect));
      Call(game, event, UI_BOX, 4, 0x86, 0x22, 0xe0, 7);
      if ((side_control[0] == SIDE_PLAYED && side_control[1] == SIDE_COMPUTER) || (side_control[0] == SIDE_COMPUTER && side_control[1] == SIDE_PLAYED)) goto difficulty;
      goto battles;
    }
    if (side_control[0] == SIDE_COMPUTER && side_control[1] == SIDE_COMPUTER) {
      Call(game, event, CONFIRM_GENERALS);
      Call(game, event, UI_BOX, 2, 0x86, 0x22, 0x154, 7);
      Call(game, event, POSITION, 4, 0x154);
      if (Call(game, event, YES_NO, at(data.scensel_ready_demo))) goto start;
      goto restart;
    }
    Call(game, event, DELAY, 15);
    Call(game, event, UI_TEXT, at(data.scensel_ready_colors));
    Call(game, event, CONFIRM_GENERALS);
    Call(game, event, POSITION, 0x14, 0x170);
  } else if (selection == 2) {
    new_game = 0;
    Call(game, event, UI_TEXT, at(data.scensel_load_colors));
    if (!Call(game, event, LOAD_SAVED_GAME)) goto restart;
    Call(game, event, POSITION, 0x1c, 0xbc);
  } else
    goto title;
  Call(game, event, STAFF_RESTORE_BOTTOM);
  Call(game, event, TEXT_ATTRIBUTE, 4);
  Call(game, event, UI_TEXT, Call(game, event, FRAMED_TEXT, 0x1c, 0xbc, at(data.scensel_ready_blank)));
  Call(game, event, POSITION, 0x1c, 0xbc);
  if (!Call(game, event, YES_NO, at(data.scensel_ready))) goto restart;
start:
  Call(game, event, LOAD_SCENARIO_GRAPHICS);
  Call(game, event, UI_TEXT, at(data.scensel_done_colors));
  Call(game, event, CLOSE, handle);
  return new_game;
}

// The services in the native build.
uint16_t svc_scensel(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case PRINT:                  opeu_cursor_print(game, { args, count }); break;
  case FORMAT:                 opeu_request_format(game, { args, count }); break;
  case POSITION:               opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case UI_TEXT:                opeu_ui_text(game, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case PROMPT_CANCEL:          r = port_prompt_cancel(game, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case PROMPT_REQUIRED:        r = port_prompt_required(game, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case YES_NO:                 r = port_prompt_yes_no(game, Arg(args, count, 0)); break;
  case OPEN:                   r = (uint16_t)opeu_open_resource(game, Arg(args, count, 0)); break;
  case TURN_CONTEXT:           opeu_set_turn_context(game, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case MUSIC_STOP:             opeu_music_stop(game, svc_sound); break;
  case FINISH_INIT:            opeu_finish_side_initialization(game, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case VGA_CLEAR:              r = (uint16_t)opeu_vga_clear(); break;
  case PICTURE_MASKED:         opeu_picture_file_masked(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), (int16_t)Arg(args, count, 2), Arg(args, count, 3)); break;
  case TITLE_PALETTE:          opeu_palette_title_screen(game, (const UiHost*)port_uiprim_host()); break;
  case STRING_COPY:            r = opeu_string_copy(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case STRING_APPEND:          r = opeu_string_append(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case UI_BOX:                 r = (uint16_t)opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case DELAY:                  opeu_delay_units(game, (uint8_t)Arg(args, count, 0)); break;
  case TEXT_ATTRIBUTE:         opeu_text_paper_set(game, (TextHost*)port_textout_host(), (uint8_t)Arg(args, count, 0)); break;
  case FRAMED_TEXT:            r = opeu_title_box(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case CLOSE:                  r = (uint16_t)opeu_dos_close((int16_t)Arg(args, count, 0)); break;
  case LOAD_SCENARIO:          port_load_scenario(game, Arg(args, count, 0)); break;
  case MARKER_SHOW:            opeu_setup_marker_show(game, svc_setmark, (uint8_t)Arg(args, count, 0)); break;
  case MARKER_HIDE:            opeu_setup_marker_hide(game, svc_setmark, (uint8_t)Arg(args, count, 0)); break;
  case LOAD_SCENARIO_TEXT:     opeu_load_scenario_text(game, (uint8_t)Arg(args, count, 0), Arg(args, count, 1)); break;
  case LOAD_MESSAGE_GRAPHICS:  opeu_load_message_graphics(game); break;
  case LOAD_SCENARIO_GRAPHICS: opeu_load_scenario_graphics(game); break;
  case LOAD_SAVED_GAME:        r = opeu_load_saved_game(game); break;
  case STAFF_RESTORE_BOTTOM:   opeu_staff_restore_bottom(game, svc_staffpnl); break;
  case CONFIRM_GENERALS:       opeu_confirm_generals(game, svc_genconf); break;
  case SCENARIO_FRAME:         opeu_scenario_frame(game, svc_scenui); break;
  case SCENARIO_MESSAGE:       opeu_scenario_message(game, svc_scenui, Arg(args, count, 0)); break;
  case CLEAR_PANEL:            opeu_scenario_clear_panel(game, svc_scenui); break;
  case CLEAR_TEXT:             opeu_scenario_clear_text(game, svc_scenui); break;
  case SCENARIO_DESCRIPTION:   opeu_scenario_description(game, svc_scenui, Arg(args, count, 0)); break;
  default:                     port_unhandled("scensel", id, count);
  }
  return r;
}
