// The Settings command: the battle, delay, map, music and picture options, saving the game, a new game and leaving
// to DOS.
#include "core/settings.hpp"
#include "common/dos-file.hpp"
#include "common/events.hpp"
#include "common/host.hpp"
#include "core/strings.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/saved-game.hpp"
#include "core/app.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/scenend.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/keyboard.hpp"
#include "io/listdlg.hpp"
#include "io/rebase.hpp"
#include "io/resopen.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>
#include <iterator>
#include <utility>

using namespace opeu::game;

namespace {

// The services the Settings routines call through their event. Strings and buffers are passed as DS offsets.
enum class SettingsService : uint16_t {
  PRINT           = 0, // (format, ...) to the text cursor
  FORMAT          = 1, // (destination, format, value)
  DIALOG          = 2, // (text, x, person, menu): the chosen entry, 0xffff when cancelled
  NUMBER          = 3, // (text, x, person, 0): a number the player enters
  MESSAGE         = 4, // (text, person)
  ASK             = 5, // (text, person): nonzero for yes
  LOCATION        = 6, // (x, y, side): show the map there
  MUSIC_STOP      = 7,
  PRE_CONTEXT     = 8, // restart the music for the turn
  SCREEN_RESET    = 9,
  ATTRIBUTE       = 10, // (paper)
  DISK_RESET      = 11,
  FRAMED          = 12, // (x, y, text): a framed title, returns the text
  UI_TEXT         = 13, // (text)
  UI_WAIT         = 14, // (time)
  OPEN            = 15, // (name): the file handle
  MENU_CHOOSE     = 16, // (menu, first, count)
  OUTLINE         = 17, // (x1, y1, x2, y2, colour)
  POSITION        = 18, // (x, y) of the text cursor
  TITLE           = 19, // (text)
  YES_NO          = 20, // (text)
  ACTIVE_BEGIN    = 21, // the side's turn display again
  PALETTE_TITLE   = 22,
  PALETTE_DEFAULT = 23,
  FINISH_SIDE     = 24, // (value)
  TURN_CONTEXT    = 25, // (value)
  FREE_SEGMENTS   = 26,
  CLOSE_FILES     = 28,
  APPLICATION     = 29, // start the game over
  CREATE          = 30, // (name): the file handle
  EXISTS          = 31, // (name)
  WRITE           = 32, // (handle, buffer, size)
  READ            = 33, // (handle, buffer, size)
  READ_FAR        = 34, // (handle, segment, offset, size)
  CLOSE           = 35, // (handle)
  SEEK            = 36, // (handle, offset low, offset high, whence)
  BOX             = 37, // (x, y, width, height, colour)
  VGA_COPY        = 38, // six words
  DRAW            = 39, // (x, y, width, height, plane, segment, offset) a bitmap
  FM_WAIT         = 40, // (time)
  FM_BUSY         = 41,
  SOUND_CLOSE     = 42, // (0, 0)
  FM_NOOP         = 43,
  TEXT_MODE       = 44,
  EXIT            = 45, // (code)
  VGA_CLEAR       = 46,
  FAR_ACTION      = 47, // (offset, segment) of an action table entry outside this module
};
using enum SettingsService;

// The entries of the far action tables (save_quit_actions, settings_actions) by their code offset.
enum class Action : uint16_t {
  BATTLES   = 0x0084,
  DELAY     = 0x00ec,
  MAP       = 0x012a,
  SOUND     = 0x0176,
  PICTURES  = 0x01cc,
  SAVE_GAME = 0x091e,
  LEAVE     = 0x0ca6,
  NEW_GAME  = 0x0cbc,
  SAVE_MENU = 0x0d32,
};

// The slot records and saved blocks of the save file.
constexpr uint16_t SLOT_BYTES{ sizeof(SaveSlot) };
constexpr auto     SAVE_FILE_SLOTS{ SAVE_SLOT_COUNT };

// The KOEI logo screen of KOEILOGO.FNT: two images read into the bitmap segment.
constexpr uint16_t LOGO_AT{ 0xbca0 }; // the logo, 208 x 112, at the start of the file
constexpr uint16_t LOGO_BYTES{ 0xb60 };
constexpr int32_t  CAPTION_IN_FILE{ 0x1ad1 }; // the caption, 320 x 40
constexpr uint16_t CAPTION_AT{ 0xafa0 };
constexpr uint16_t CAPTION_BYTES{ 0x640 };

// The exit code the game ends with when the player leaves to DOS.
constexpr uint16_t QUIT_EXIT_CODE{ 100 };

// SEEK with a 32-bit file offset.
auto Seek(Game& game, HostEvent event, uint16_t handle, int32_t offset, uint16_t whence) -> uint16_t {
  return Call(game, event, SEEK, handle, static_cast<uint16_t>(offset), static_cast<uint16_t>(static_cast<uint32_t>(offset) >> 16), whence);
}

// Runs the save/quit menu's entry `index`.
void SaveMenuAction(Game& game, HostEvent event, uint16_t index) {
  auto const& entry{ Data(game).save_quit_actions[index] };
  switch (static_cast<Action>(entry.offset)) {
  case Action::SAVE_GAME: opeu_save_game(game, event); break;
  case Action::NEW_GAME:  opeu_settings_new_game(game, event); break;
  case Action::LEAVE:     opeu_settings_leave(game, event); break;
  default:                Call(game, event, FAR_ACTION, entry.offset, entry.segment);
  }
}

// Runs the Settings menu's entry `index`; only the save/quit menu's result is examined.
auto SettingsAction(Game& game, HostEvent event, uint16_t index) -> uint16_t {
  auto const& entry{ Data(game).settings_actions[index] };
  switch (static_cast<Action>(entry.offset)) {
  case Action::BATTLES:   opeu_settings_battles(game, event); break;
  case Action::DELAY:     opeu_settings_delay(game, event); break;
  case Action::MAP:       opeu_settings_map(game, event); break;
  case Action::SOUND:     opeu_settings_sound(game, event); break;
  case Action::PICTURES:  opeu_settings_pictures(game, event); break;
  case Action::SAVE_MENU: return opeu_settings_save_menu(game, event);
  default:                return Call(game, event, FAR_ACTION, entry.offset, entry.segment);
  }
  return 0;
}

// Fills a slot record with the date, scenario and control of the game in progress.
void FillSlot(DataSegment& data, SaveSlot& slot) {
  slot.scenario   = data.scenario;
  slot.delivery   = data.delivery_level;
  slot.year       = data.year;
  slot.month      = data.month;
  slot.day        = data.day;
  slot.hour       = data.hour;
  slot.control[0] = data.side_control[0];
  slot.control[1] = data.side_control[1];
}

} // namespace

// The current settings, printed under the Settings title.
void opeu_settings_print(Game& game, HostEvent event) {
  auto const& data{ Data(game) };
  auto const  battles{ static_cast<BattleView>(data.battle_view) };
  Call(game, event, PRINT, Near(data, data.settings_summary), data.message_delay,
       data.sound_enabled == 1 && data.sound_driver ? Near(data, data.settings_music_on) : Near(data, data.settings_music_off),
       battles == BattleView::ALL    ? Near(data, data.settings_battles_view)
       : battles == BattleView::NONE ? Near(data, data.settings_battles_no)
                                     : Near(data, data.settings_battles_select),
       data.event_pictures == 1 ? Near(data, data.settings_pictures_on) : Near(data, data.settings_pictures_off),
       !data.map_mode[0] ? Near(data, data.settings_aerial) : Near(data, data.settings_field));
}

// Which battles to watch: all, none or a choice before each.
void opeu_settings_battles(Game& game, HostEvent event) {
  ServiceLocals const locals{ 102 }; // the message the format service writes
  auto&               data{ Data(game) };
  uint16_t const      message{ locals.At(0) };
  auto const          choice{ Call(game, event, DIALOG, Near(data, data.battles_question), 3, data.context_record2, Near(data, data.battles_menu)) };
  switch (choice) {
  case 1:  data.battle_view = std::to_underlying(BattleView::ALL); break;
  case 2:  data.battle_view = std::to_underlying(BattleView::NONE); break;
  case 3:  data.battle_view = std::to_underlying(BattleView::SELECT); break;
  default: return;
  }
  Call(game, event, FORMAT, message, Near(data, data.battles_reply), data.battles_descriptions[choice - 1]);
  Call(game, event, MESSAGE, message, data.context_record2);
}

// How long messages stay on screen.
void opeu_settings_delay(Game& game, HostEvent event) {
  ServiceLocals const locals{ 100 }; // the message the format service writes
  auto&               data{ Data(game) };
  uint16_t const      message{ locals.At(0) };
  data.message_delay = static_cast<uint8_t>(Call(game, event, NUMBER, Near(data, data.delay_question), 10, data.context_record2, 0));
  Call(game, event, FORMAT, message, Near(data, data.delay_reply), data.message_delay);
  Call(game, event, MESSAGE, message, data.context_record2);
}

// Switches between the aerial and the field map and shows the current unit's place on it.
void opeu_settings_map(Game& game, HostEvent event) {
  ServiceLocals const locals{ 100 }; // the message the format service writes
  auto&               data{ Data(game) };
  uint16_t const      message{ locals.At(0) };
  data.map_mode[0]++;
  data.map_mode[0] &= 1;
  Call(game, event, FORMAT, message, Near(data, data.map_reply), !data.map_mode[0] ? Near(data, data.map_aerial) : Near(data, data.map_field));
  Call(game, event, MESSAGE, message, data.context_record2);
  auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
  Call(game, event, LOCATION, unit.x, unit.y, data.current_side);
}

// Turns the music off or on.
void opeu_settings_sound(Game& game, HostEvent event) {
  ServiceLocals const locals{ 100 }; // the message the format service writes
  auto&               data{ Data(game) };
  uint16_t const      message{ locals.At(0) };
  if (data.sound_enabled == 1) {
    Call(game, event, MUSIC_STOP);
    data.sound_enabled = 0;
  } else {
    data.sound_enabled = 1;
    Call(game, event, PRE_CONTEXT);
  }
  Call(game, event, FORMAT, message, Near(data, data.sound_reply), data.sound_enabled == 1 ? Near(data, data.sound_on) : Near(data, data.sound_off));
  Call(game, event, MESSAGE, message, data.context_record2);
}

// Turns the event pictures off or on.
void opeu_settings_pictures(Game& game, HostEvent event) {
  ServiceLocals const locals{ 100 }; // the message the format service writes
  auto&               data{ Data(game) };
  uint16_t const      message{ locals.At(0) };
  data.event_pictures = data.event_pictures == 1 ? 0 : 1;
  Call(game, event, FORMAT, message, Near(data, data.pictures_reply), data.event_pictures == 1 ? Near(data, data.pictures_will) : Near(data, data.pictures_will_not));
  Call(game, event, MESSAGE, message, data.context_record2);
}

// Turns the record indices of a saved block back into near pointers (0xffff becomes null).
void opeu_save_rebase(Game& game) {
  opeu_rebase_scenario(game, 1);
}

// Turns the record pointers of the game into the indices a saved block stores (null becomes 0xffff).
void opeu_save_unrebase(Game& game) {
  auto& data{ Data(game) };
  for (auto& side : data.sides) {
    if (auto const target{ SideOrderTarget(side.order) }; target != OrderTarget::NONE) side.order.target = StoredTarget(side.order.target, target);
    for (auto& objective : side.objectives) objective.target = StoredIndex<Objective>(objective.target, OBJECTIVES_AT);
  }
  for (auto& special : data.special) special.target = StoredIndex<Objective>(special.target, OBJECTIVES_AT);
  // word arrays of the packed segment: indexed, as a reference cannot bind to their elements
  for (auto i{ 0u }; i < std::size(data.objective_links); i++) data.objective_links[i] = StoredIndex<Objective>(data.objective_links[i], OBJECTIVES_AT);
  for (auto i{ 0u }; i < std::size(data.unit_links); i++) data.unit_links[i] = StoredIndex<Unit>(data.unit_links[i], UNITS_AT);
  for (auto& unit : data.units)
    for (auto& order : unit.orders)
      if (UnitOrderTarget(order) == OrderTarget::UNIT) order.target = StoredIndex<Unit>(order.target, UNITS_AT);
  for (auto& unit : data.units) unit.first = StoredIndex<Element>(unit.first, ELEMENTS_AT);
  for (auto& element : data.elements) element.next = StoredIndex<Element>(element.next, ELEMENTS_AT);
}

// The byte sum of the saved block into the slot record.
void opeu_save_checksum(Game& game, uint8_t slot) {
  auto& data{ Data(game) };
  data.save_slots[slot].checksum = SavedChecksum(data);
}

// The first save: a new file with twenty slot records and twenty copies of the game. The first slot's record is
// written for every slot, its scenario marking the others empty, so that record is left marked empty.
void opeu_save_create(Game& game, HostEvent event) {
  auto& data{ Data(game) };
  Call(game, event, SCREEN_RESET);
  Call(game, event, ATTRIBUTE, 4);
  Call(game, event, DISK_RESET);
  Call(game, event, UI_TEXT, Call(game, event, FRAMED, 32, 188, Near(data, data.create_saving)));
  Call(game, event, DISK_RESET);
  auto& first{ data.save_slots[0] };
  FillSlot(data, first);
  opeu_save_checksum(game, 0);
  auto const file{ Call(game, event, CREATE, Near(data, data.create_file_name)) };
  for (uint8_t slot = 0; slot < SAVE_FILE_SLOTS; slot++) {
    first.scenario = slot == 0 ? data.scenario : SLOT_EMPTY;
    Call(game, event, WRITE, file, SAVE_SLOTS_AT, SLOT_BYTES);
  }
  opeu_save_unrebase(game);
  for (uint8_t slot = 0; slot < SAVE_FILE_SLOTS; slot++) Call(game, event, WRITE, file, SAVED_FIRST, SAVED_BYTES);
  opeu_save_rebase(game);
  Call(game, event, CLOSE, file);
  if (Call(game, event, EXISTS, Near(data, data.create_check_name))) {
    Call(game, event, UI_TEXT, Call(game, event, FRAMED, 31, 188, Near(data, data.create_saved)));
    Call(game, event, UI_WAIT, 15);
    Call(game, event, ATTRIBUTE, 0);
  }
}

// A framed prompt at text column x, pixel row y, with the cursor at its start; returns the text.
uint16_t opeu_save_prompt_box(Game& game, HostEvent event, uint16_t x, uint16_t y, uint16_t text) {
  auto const width{ static_cast<uint16_t>(opeu_string_length(game, text) + 2) };
  Call(game, event, BOX, x - 1, y - 4, width, 24, 1);
  Call(game, event, OUTLINE, (x - 1) * 8, y - 4, (x + width) * 8 - 9, y + 19, 7);
  Call(game, event, POSITION, x, y);
  return text;
}

// Saves the game into a slot the player picks (creating the file on the first save).
void opeu_save_game(Game& game, HostEvent event) {
  auto& data{ Data(game) };
  Call(game, event, SCREEN_RESET);
  Call(game, event, ATTRIBUTE, 4);
  if (!Call(game, event, EXISTS, Near(data, data.save_check_name))) {
    opeu_save_create(game, event);
    if (!Call(game, event, EXISTS, Near(data, data.save_recheck_name))) {
      Call(game, event, PRINT, Near(data, data.save_failed_format), Call(game, event, FRAMED, 30, 188, Near(data, data.save_failed)));
      Call(game, event, UI_WAIT, 20);
      Call(game, event, ATTRIBUTE, 0);
    }
  } else {
    Call(game, event, ATTRIBUTE, 0);
    auto const file{ Call(game, event, OPEN, Near(data, data.save_open_name)) };
    for (uint8_t slot = 0; slot < SAVE_FILE_SLOTS; slot++) Call(game, event, READ, file, Near(data, data.save_slots[slot]), SLOT_BYTES);
    for (;;) {
      Call(game, event, ATTRIBUTE, 4);
      Call(game, event, UI_TEXT, Call(game, event, FRAMED, 36, 336, Near(data, data.save_where)));
      Call(game, event, ATTRIBUTE, 0);
      if (!Call(game, event, MENU_CHOOSE, Near(data, data.save_list), 1, 3)) {
        Call(game, event, CLOSE, file);
        break;
      }
      Call(game, event, ATTRIBUTE, 4);
      Call(game, event, VGA_COPY, 0, 240, 80, 48, 0, 320);
      Call(game, event, UI_TEXT, opeu_save_prompt_box(game, event, 35, 336, Near(data, data.save_confirm)));
      Call(game, event, POSITION, 39, 336);
      if (!Call(game, event, YES_NO, Near(data, data.save_answer))) continue;
      Call(game, event, UI_TEXT, Call(game, event, FRAMED, 35, 188, Near(data, data.save_saving)));
      auto& slot{ data.save_slots[data.save_slot] };
      FillSlot(data, slot);
      opeu_save_checksum(game, data.save_slot);
      Seek(game, event, file, static_cast<int32_t>(data.save_slot) * SLOT_BYTES, SEEK_FROM_START);
      Call(game, event, WRITE, file, Near(data, slot), SLOT_BYTES);
      Seek(game, event, file, static_cast<int32_t>(SAVED_BYTES) * data.save_slot + SAVED_BLOCKS_AT, SEEK_FROM_START);
      opeu_save_unrebase(game);
      Call(game, event, WRITE, file, SAVED_FIRST, SAVED_BYTES);
      opeu_save_rebase(game);
      Call(game, event, CLOSE, file);
      Call(game, event, UI_TEXT, Call(game, event, FRAMED, 35, 188, Near(data, data.save_finished)));
      Call(game, event, UI_WAIT, 20);
      Call(game, event, ATTRIBUTE, 0);
      break;
    }
  }
  Call(game, event, SCREEN_RESET);
  auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
  Call(game, event, LOCATION, unit.x, unit.y, data.current_side);
  Call(game, event, ACTIVE_BEGIN);
}

// Shows the KOEI logo, shuts the sound and memory down and exits with code 100.
void opeu_quit_to_dos(Game& game, HostEvent event) {
  auto const& data{ Data(game) };
  auto const  file{ Call(game, event, OPEN, Near(data, data.logo_file_name)) };
  Seek(game, event, file, 0, SEEK_FROM_START);
  Call(game, event, READ_FAR, file, data.bitmap_buffer, LOGO_AT, LOGO_BYTES);
  Seek(game, event, file, CAPTION_IN_FILE, SEEK_FROM_START);
  Call(game, event, READ_FAR, file, data.bitmap_buffer, CAPTION_AT, CAPTION_BYTES);
  Call(game, event, CLOSE, file);
  Call(game, event, VGA_CLEAR);
  Call(game, event, PALETTE_TITLE);
  Call(game, event, FINISH_SIDE, 0);
  Call(game, event, DRAW, 216, 120, 208, 112, 2, data.bitmap_buffer, LOGO_AT);
  Call(game, event, DRAW, 160, 232, 320, 40, 7, data.bitmap_buffer, CAPTION_AT);
  Call(game, event, TURN_CONTEXT, 0);
  Call(game, event, MUSIC_STOP);
  Call(game, event, FM_WAIT, 200);
  while (Call(game, event, FM_BUSY));
  Call(game, event, SOUND_CLOSE, 0, 0);
  Call(game, event, FM_NOOP);
  Call(game, event, TEXT_MODE);
  Call(game, event, FREE_SEGMENTS);
  Call(game, event, DISK_RESET);
  Call(game, event, EXIT, QUIT_EXIT_CODE);
}

void opeu_settings_leave(Game& game, HostEvent event) {
  auto const& data{ Data(game) };
  if (Call(game, event, ASK, Near(data, data.leave_question), data.context_record2)) opeu_quit_to_dos(game, event);
}

// Starts a new game; the question is skipped when both sides are computer controlled.
void opeu_settings_new_game(Game& game, HostEvent event) {
  auto& data{ Data(game) };
  if ((data.side_control[0] == SIDE_COMPUTER && data.side_control[1] == SIDE_COMPUTER) || Call(game, event, ASK, Near(data, data.new_game_question), data.context_record2)) {
    Call(game, event, TURN_CONTEXT, 0);
    Call(game, event, MUSIC_STOP);
    Call(game, event, SOUND_CLOSE, 0, 0);
    Call(game, event, CLOSE_FILES);
    Call(game, event, PALETTE_DEFAULT);
    Call(game, event, FM_NOOP);
    Call(game, event, TEXT_MODE);
    Call(game, event, FREE_SEGMENTS);
    Call(game, event, VGA_CLEAR);
    Call(game, event, SCREEN_RESET);
    data.startup_mode  = 0;
    data.daily_pending = 1;
    data.restart_flag  = 0;
    Call(game, event, APPLICATION);
  }
}

// The save/quit menu; returns the choice, 0xffff when cancelled.
uint16_t opeu_settings_save_menu(Game& game, HostEvent event) {
  auto const& data{ Data(game) };
  auto const  choice{ Call(game, event, DIALOG, Near(data, data.save_quit_question), 3, data.context_record2, Near(data, data.save_quit_menu)) };
  if (choice == 0xffff) return choice;
  SaveMenuAction(game, event, choice - 1);
  return choice;
}

// The Settings command: the options menu until it is cancelled or the save/quit entry returns 1.
void opeu_turn_action_10(Game& game, HostEvent event) {
  auto const& data{ Data(game) };
  for (;;) {
    Call(game, event, TITLE, Near(data, data.settings_title));
    opeu_settings_print(game, event);
    auto const choice{ Call(game, event, DIALOG, Near(data, data.settings_question), 6, data.context_record2, Near(data, data.settings_menu)) };
    if (choice == 0xffff) return;
    auto const result{ SettingsAction(game, event, choice - 1) };
    if (choice == 6 && result == 1) return;
  }
}

// Each service of the Settings routines is the direct call of the routine that provides it.
uint16_t svc_settings(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (static_cast<SettingsService>(id)) {
  case PRINT:           opeu_cursor_print(game, { args, count }); break;
  case FORMAT:          opeu_request_format(game, { args, count }); break;
  case DIALOG:          r = opeu_delay_dialog(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case NUMBER:          r = (uint8_t)opeu_choose_dialog(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case MESSAGE:         opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case ASK:             r = opeu_confirm_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case LOCATION:        opeu_arrival_location(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case MUSIC_STOP:      opeu_music_stop(game, svc_sound); break;
  case PRE_CONTEXT:     opeu_turn_pre_context(game); break;
  case SCREEN_RESET:    opeu_screen_reset(game, (const DialogHost*)port_dialogs_host()); break;
  case ATTRIBUTE:       opeu_text_paper_set(game, (TextHost*)port_textout_host(), (uint8_t)Arg(args, count, 0)); break;
  case DISK_RESET:      opeu_disk_reset(game); break;
  case FRAMED:          r = opeu_title_box(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case UI_TEXT:         opeu_ui_text(game, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case UI_WAIT:         opeu_delay_units(game, (uint8_t)Arg(args, count, 0)); break;
  case OPEN:            r = (uint16_t)opeu_open_resource(game, Arg(args, count, 0)); break;
  case MENU_CHOOSE:     r = opeu_list_dialog(game, svc_listdlg, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case OUTLINE:         opeu_ui_outline(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case POSITION:        opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case TITLE:           opeu_army_title(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case YES_NO:          r = (uint16_t)opeu_prompt_yes_no(game, svc_keyboard, Arg(args, count, 0)); break;
  case ACTIVE_BEGIN:    opeu_turn_active_begin(game, (const DialogHost*)port_dialogs_host()); break;
  case PALETTE_TITLE:   opeu_palette_title_screen(game, (const UiHost*)port_uiprim_host()); break;
  case PALETTE_DEFAULT: opeu_palette_daylight(game, (const UiHost*)port_uiprim_host()); break;
  case FINISH_SIDE:     opeu_finish_side_initialization(game, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case TURN_CONTEXT:    opeu_set_turn_context(game, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case FREE_SEGMENTS:   opeu_free_segments(game, svc_scenend); break;
  case CLOSE_FILES:     opeu_close_data_files(game, svc_scenend); break;
  case APPLICATION:     opeu_application_entry(game, svc_app); break;
  case CREATE:          r = (uint16_t)opeu_dos_create((const char*)game.State().Bytes(Arg(args, count, 0)).data()); break;
  case EXISTS:          r = (uint16_t)opeu_dos_exists((const char*)game.State().Bytes(Arg(args, count, 0)).data()); break;
  case WRITE:           r = (uint16_t)opeu_dos_write((int16_t)Arg(args, count, 0), game.State().Bytes(Arg(args, count, 1), Arg(args, count, 2)).data(), Arg(args, count, 2)); break;
  case READ:            r = (uint16_t)opeu_dos_read((int16_t)Arg(args, count, 0), game.State().Bytes(Arg(args, count, 1), Arg(args, count, 2)).data(), Arg(args, count, 2)); break;
  case READ_FAR:        r = (uint16_t)opeu_dos_read_far((int16_t)Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case CLOSE:           r = (uint16_t)opeu_dos_close((int16_t)Arg(args, count, 0)); break;
  case SEEK:            r = (uint16_t)opeu_dos_seek((int16_t)Arg(args, count, 0), (int32_t)Arg32(args, count, 1), (int16_t)Arg(args, count, 3)); break;
  case BOX:             r = (uint16_t)opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case VGA_COPY:        opeu_screen_copy(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), Arg(args, count, 5)); break;
  case DRAW:
    r = (uint16_t)opeu_gfx_blit(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), port_far(Arg(args, count, 5), Arg(args, count, 6)));
    break;
  case FM_WAIT: opeu_fm_wait_start(Arg(args, count, 0)); break;
  case FM_BUSY: r = opeu_fm_busy(); break;
  // sound_open(0L): one null far pointer; the effects pointer words the routine also reads are passed as 0
  case SOUND_CLOSE: r = (uint16_t)opeu_sound_open(Arg(args, count, 0), Arg(args, count, 1), 0, 0); break;
  case FM_NOOP:     opeu_fm_noop(); break;
  case TEXT_MODE:   opeu_text_mode(); break;
  case EXIT:        port_exit((int16_t)Arg(args, count, 0)); break;
  case VGA_CLEAR:   r = (uint16_t)opeu_vga_clear(); break;
  case FAR_ACTION:  r = (uint16_t)port_call(game, (uint32_t)Arg(args, count, 1) * 16u + Arg(args, count, 0), 0, 0); break;
  default:          port_unhandled("settings", id, count);
  }
  return r;
}

auto port_routine_opeu_turn_action_10(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_turn_action_10(game, svc_settings);
  return 0;
}
