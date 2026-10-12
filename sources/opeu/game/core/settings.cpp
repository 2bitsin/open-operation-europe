// The Settings command: the battle, delay, map, music and picture options, saving the game, a new game and leaving
// to DOS.
#include "core/settings.hpp"
#include "common/events.hpp"
#include "common/host.hpp"
#include "core/strings.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/saved-game.hpp"
#include "core/app.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/emsmap.hpp"
#include "core/scenend.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "io/keyboard.hpp"
#include "io/listdlg.hpp"
#include "io/rebase.hpp"
#include "io/resopen.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
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
  MEMORY_RELEASE  = 27,
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

// An option routine's message buffer: `size` bytes below its frame on the DS stack (sp is its entry SP).
constexpr auto MessageBuffer(uint16_t sp, uint16_t size) -> uint16_t {
  return Local(static_cast<uint16_t>(sp - 2), size);
}

// The entry SP of an option called from the Settings command: below its frame and the call.
constexpr uint16_t OPTION_SP_DEPTH{ 14 };

// The caller scratch area the yes/no prompt reads keys into.

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
auto Seek(uint8_t* ds, HostEvent event, uint16_t handle, int32_t offset, uint16_t whence) -> uint16_t {
  return Call(ds, event, SEEK, handle, static_cast<uint16_t>(offset), static_cast<uint16_t>(static_cast<uint32_t>(offset) >> 16), whence);
}

// A stored record index (0xffff for none) from a near pointer to the table of T at `base` (0 for none). The pointer
// difference is divided as a signed word.
template <typename T>
auto StoredIndex(uint16_t near, uint16_t base) -> uint16_t {
  if (near == 0) return 0xffff;
  return static_cast<uint16_t>(static_cast<int16_t>(static_cast<uint16_t>(near - base)) / static_cast<int16_t>(sizeof(T)));
}

auto StoredTarget(uint16_t near, OrderTarget target) -> uint16_t {
  return target == OrderTarget::OBJECTIVE ? StoredIndex<Objective>(near, OBJECTIVES_AT) : StoredIndex<Unit>(near, UNITS_AT);
}

// Runs the save/quit menu's entry `index`.
void SaveMenuAction(uint8_t* ds, HostEvent event, uint16_t index) {
  auto const& entry{ Data(ds).save_quit_actions[index] };
  switch (static_cast<Action>(entry.offset)) {
  case Action::SAVE_GAME: opeu_save_game(ds, event); break;
  case Action::NEW_GAME:  opeu_settings_new_game(ds, event); break;
  case Action::LEAVE:     opeu_settings_leave(ds, event); break;
  default:                Call(ds, event, FAR_ACTION, entry.offset, entry.segment);
  }
}

// Runs the Settings menu's entry `index` with its entry SP; only the save/quit menu's result is examined.
auto SettingsAction(uint8_t* ds, HostEvent event, uint16_t entry_sp, uint16_t index) -> uint16_t {
  auto const& entry{ Data(ds).settings_actions[index] };
  switch (static_cast<Action>(entry.offset)) {
  case Action::BATTLES:   opeu_settings_battles(ds, event, entry_sp); break;
  case Action::DELAY:     opeu_settings_delay(ds, event, entry_sp); break;
  case Action::MAP:       opeu_settings_map(ds, event, entry_sp); break;
  case Action::SOUND:     opeu_settings_sound(ds, event, entry_sp); break;
  case Action::PICTURES:  opeu_settings_pictures(ds, event, entry_sp); break;
  case Action::SAVE_MENU: return opeu_settings_save_menu(ds, event);
  default:                return Call(ds, event, FAR_ACTION, entry.offset, entry.segment);
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
void opeu_settings_print(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  auto const  battles{ static_cast<BattleView>(data.battle_view) };
  Call(ds, event, PRINT, Near(data, data.settings_summary), data.message_delay, data.sound_enabled == 1 && data.sound_driver ? Near(data, data.settings_music_on) : Near(data, data.settings_music_off),
       battles == BattleView::ALL    ? Near(data, data.settings_battles_view)
       : battles == BattleView::NONE ? Near(data, data.settings_battles_no)
                                     : Near(data, data.settings_battles_select),
       data.event_pictures == 1 ? Near(data, data.settings_pictures_on) : Near(data, data.settings_pictures_off),
       !data.map_mode[0] ? Near(data, data.settings_aerial) : Near(data, data.settings_field));
}

// Which battles to watch: all, none or a choice before each.
void opeu_settings_battles(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&          data{ Data(ds) };
  uint16_t const message{ MessageBuffer(sp, 102) };
  auto const     choice{ Call(ds, event, DIALOG, Near(data, data.battles_question), 3, data.context_record2, Near(data, data.battles_menu)) };
  switch (choice) {
  case 1:  data.battle_view = std::to_underlying(BattleView::ALL); break;
  case 2:  data.battle_view = std::to_underlying(BattleView::NONE); break;
  case 3:  data.battle_view = std::to_underlying(BattleView::SELECT); break;
  default: return;
  }
  Call(ds, event, FORMAT, message, Near(data, data.battles_reply), data.battles_descriptions[choice - 1]);
  Call(ds, event, MESSAGE, message, data.context_record2);
}

// How long messages stay on screen.
void opeu_settings_delay(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&          data{ Data(ds) };
  uint16_t const message{ MessageBuffer(sp, 100) };
  data.message_delay = static_cast<uint8_t>(Call(ds, event, NUMBER, Near(data, data.delay_question), 10, data.context_record2, 0));
  Call(ds, event, FORMAT, message, Near(data, data.delay_reply), data.message_delay);
  Call(ds, event, MESSAGE, message, data.context_record2);
}

// Switches between the aerial and the field map and shows the current unit's place on it.
void opeu_settings_map(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&          data{ Data(ds) };
  uint16_t const message{ MessageBuffer(sp, 100) };
  data.map_mode[0]++;
  data.map_mode[0] &= 1;
  Call(ds, event, FORMAT, message, Near(data, data.map_reply), !data.map_mode[0] ? Near(data, data.map_aerial) : Near(data, data.map_field));
  Call(ds, event, MESSAGE, message, data.context_record2);
  auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
  Call(ds, event, LOCATION, unit.x, unit.y, data.current_side);
}

// Turns the music off or on.
void opeu_settings_sound(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&          data{ Data(ds) };
  uint16_t const message{ MessageBuffer(sp, 100) };
  if (data.sound_enabled == 1) {
    Call(ds, event, MUSIC_STOP);
    data.sound_enabled = 0;
  } else {
    data.sound_enabled = 1;
    Call(ds, event, PRE_CONTEXT);
  }
  Call(ds, event, FORMAT, message, Near(data, data.sound_reply), data.sound_enabled == 1 ? Near(data, data.sound_on) : Near(data, data.sound_off));
  Call(ds, event, MESSAGE, message, data.context_record2);
}

// Turns the event pictures off or on.
void opeu_settings_pictures(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&          data{ Data(ds) };
  uint16_t const message{ MessageBuffer(sp, 100) };
  data.event_pictures = data.event_pictures == 1 ? 0 : 1;
  Call(ds, event, FORMAT, message, Near(data, data.pictures_reply), data.event_pictures == 1 ? Near(data, data.pictures_will) : Near(data, data.pictures_will_not));
  Call(ds, event, MESSAGE, message, data.context_record2);
}

// Turns the record indices of a saved block back into near pointers (0xffff becomes null).
void opeu_save_rebase(uint8_t ds[65536]) {
  opeu_rebase_scenario(ds, 1);
}

// Turns the record pointers of the game into the indices a saved block stores (null becomes 0xffff).
void opeu_save_unrebase(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
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
void opeu_save_checksum(uint8_t ds[65536], uint8_t slot) {
  auto& data{ Data(ds) };
  data.save_slots[slot].checksum = SavedChecksum(data);
}

// The first save: a new file with twenty slot records and twenty copies of the game. The first slot's record is
// written for every slot, its scenario marking the others empty, so that record is left marked empty.
void opeu_save_create(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  Call(ds, event, SCREEN_RESET);
  Call(ds, event, ATTRIBUTE, 4);
  Call(ds, event, DISK_RESET);
  Call(ds, event, UI_TEXT, Call(ds, event, FRAMED, 32, 188, Near(data, data.create_saving)));
  Call(ds, event, DISK_RESET);
  auto& first{ data.save_slots[0] };
  FillSlot(data, first);
  opeu_save_checksum(ds, 0);
  auto const file{ Call(ds, event, CREATE, Near(data, data.create_file_name)) };
  for (uint8_t slot = 0; slot < SAVE_FILE_SLOTS; slot++) {
    first.scenario = slot == 0 ? data.scenario : SLOT_EMPTY;
    Call(ds, event, WRITE, file, SAVE_SLOTS_AT, SLOT_BYTES);
  }
  opeu_save_unrebase(ds);
  for (uint8_t slot = 0; slot < SAVE_FILE_SLOTS; slot++) Call(ds, event, WRITE, file, SAVED_FIRST, SAVED_BYTES);
  opeu_save_rebase(ds);
  Call(ds, event, CLOSE, file);
  if (Call(ds, event, EXISTS, Near(data, data.create_check_name))) {
    Call(ds, event, UI_TEXT, Call(ds, event, FRAMED, 31, 188, Near(data, data.create_saved)));
    Call(ds, event, UI_WAIT, 15);
    Call(ds, event, ATTRIBUTE, 0);
  }
}

// A framed prompt at text column x, pixel row y, with the cursor at its start; returns the text.
uint16_t opeu_save_prompt_box(uint8_t ds[65536], HostEvent event, uint16_t x, uint16_t y, uint16_t text) {
  auto const width{ static_cast<uint16_t>(opeu_string_length(ds, text) + 2) };
  Call(ds, event, BOX, x - 1, y - 4, width, 24, 1);
  Call(ds, event, OUTLINE, (x - 1) * 8, y - 4, (x + width) * 8 - 9, y + 19, 7);
  Call(ds, event, POSITION, x, y);
  return text;
}

// Saves the game into a slot the player picks (creating the file on the first save).
void opeu_save_game(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  Call(ds, event, SCREEN_RESET);
  Call(ds, event, ATTRIBUTE, 4);
  if (!Call(ds, event, EXISTS, Near(data, data.save_check_name))) {
    opeu_save_create(ds, event);
    if (!Call(ds, event, EXISTS, Near(data, data.save_recheck_name))) {
      Call(ds, event, PRINT, Near(data, data.save_failed_format), Call(ds, event, FRAMED, 30, 188, Near(data, data.save_failed)));
      Call(ds, event, UI_WAIT, 20);
      Call(ds, event, ATTRIBUTE, 0);
    }
  } else {
    Call(ds, event, ATTRIBUTE, 0);
    auto const file{ Call(ds, event, OPEN, Near(data, data.save_open_name)) };
    for (uint8_t slot = 0; slot < SAVE_FILE_SLOTS; slot++) Call(ds, event, READ, file, Near(data, data.save_slots[slot]), SLOT_BYTES);
    for (;;) {
      Call(ds, event, ATTRIBUTE, 4);
      Call(ds, event, UI_TEXT, Call(ds, event, FRAMED, 36, 336, Near(data, data.save_where)));
      Call(ds, event, ATTRIBUTE, 0);
      if (!Call(ds, event, MENU_CHOOSE, Near(data, data.save_list), 1, 3)) {
        Call(ds, event, CLOSE, file);
        break;
      }
      Call(ds, event, ATTRIBUTE, 4);
      Call(ds, event, VGA_COPY, 0, 240, 80, 48, 0, 320);
      Call(ds, event, UI_TEXT, opeu_save_prompt_box(ds, event, 35, 336, Near(data, data.save_confirm)));
      Call(ds, event, POSITION, 39, 336);
      if (!Call(ds, event, YES_NO, Near(data, data.save_answer))) continue;
      Call(ds, event, UI_TEXT, Call(ds, event, FRAMED, 35, 188, Near(data, data.save_saving)));
      auto& slot{ data.save_slots[data.save_slot] };
      FillSlot(data, slot);
      opeu_save_checksum(ds, data.save_slot);
      Seek(ds, event, file, static_cast<int32_t>(data.save_slot) * SLOT_BYTES, 0);
      Call(ds, event, WRITE, file, Near(data, slot), SLOT_BYTES);
      Seek(ds, event, file, static_cast<int32_t>(SAVED_BYTES) * data.save_slot + SAVED_BLOCKS_AT, 0);
      opeu_save_unrebase(ds);
      Call(ds, event, WRITE, file, SAVED_FIRST, SAVED_BYTES);
      opeu_save_rebase(ds);
      Call(ds, event, CLOSE, file);
      Call(ds, event, UI_TEXT, Call(ds, event, FRAMED, 35, 188, Near(data, data.save_finished)));
      Call(ds, event, UI_WAIT, 20);
      Call(ds, event, ATTRIBUTE, 0);
      break;
    }
  }
  Call(ds, event, SCREEN_RESET);
  auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
  Call(ds, event, LOCATION, unit.x, unit.y, data.current_side);
  Call(ds, event, ACTIVE_BEGIN);
}

// Shows the KOEI logo, shuts the sound and memory down and exits with code 100.
void opeu_quit_to_dos(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  auto const  file{ Call(ds, event, OPEN, Near(data, data.logo_file_name)) };
  Seek(ds, event, file, 0, 0);
  Call(ds, event, READ_FAR, file, data.bitmap_buffer, LOGO_AT, LOGO_BYTES);
  Seek(ds, event, file, CAPTION_IN_FILE, 0);
  Call(ds, event, READ_FAR, file, data.bitmap_buffer, CAPTION_AT, CAPTION_BYTES);
  Call(ds, event, CLOSE, file);
  Call(ds, event, VGA_CLEAR);
  Call(ds, event, PALETTE_TITLE);
  Call(ds, event, FINISH_SIDE, 0);
  Call(ds, event, DRAW, 216, 120, 208, 112, 2, data.bitmap_buffer, LOGO_AT);
  Call(ds, event, DRAW, 160, 232, 320, 40, 7, data.bitmap_buffer, CAPTION_AT);
  Call(ds, event, TURN_CONTEXT, 0);
  Call(ds, event, MUSIC_STOP);
  Call(ds, event, FM_WAIT, 200);
  while (Call(ds, event, FM_BUSY));
  Call(ds, event, SOUND_CLOSE, 0, 0);
  Call(ds, event, FM_NOOP);
  Call(ds, event, TEXT_MODE);
  Call(ds, event, FREE_SEGMENTS);
  Call(ds, event, MEMORY_RELEASE);
  Call(ds, event, DISK_RESET);
  Call(ds, event, EXIT, QUIT_EXIT_CODE);
}

void opeu_settings_leave(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  if (Call(ds, event, ASK, Near(data, data.leave_question), data.context_record2)) opeu_quit_to_dos(ds, event);
}

// Starts a new game; the question is skipped when both sides are computer controlled.
void opeu_settings_new_game(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  if ((data.side_control[0] == 0 && data.side_control[1] == 0) || Call(ds, event, ASK, Near(data, data.new_game_question), data.context_record2)) {
    Call(ds, event, TURN_CONTEXT, 0);
    Call(ds, event, MUSIC_STOP);
    Call(ds, event, SOUND_CLOSE, 0, 0);
    Call(ds, event, CLOSE_FILES);
    Call(ds, event, PALETTE_DEFAULT);
    Call(ds, event, FM_NOOP);
    Call(ds, event, TEXT_MODE);
    Call(ds, event, FREE_SEGMENTS);
    Call(ds, event, MEMORY_RELEASE);
    Call(ds, event, VGA_CLEAR);
    Call(ds, event, SCREEN_RESET);
    data.startup_mode  = 0;
    data.daily_pending = 1;
    data.restart_flag  = 0;
    Call(ds, event, APPLICATION);
  }
}

// The save/quit menu; returns the choice, 0xffff when cancelled.
uint16_t opeu_settings_save_menu(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  auto const  choice{ Call(ds, event, DIALOG, Near(data, data.save_quit_question), 3, data.context_record2, Near(data, data.save_quit_menu)) };
  if (choice == 0xffff) return choice;
  SaveMenuAction(ds, event, choice - 1);
  return choice;
}

// The Settings command: the options menu until it is cancelled or the save/quit entry returns 1.
void opeu_turn_action_10(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto const& data{ Data(ds) };
  for (;;) {
    Call(ds, event, TITLE, Near(data, data.settings_title));
    opeu_settings_print(ds, event);
    auto const choice{ Call(ds, event, DIALOG, Near(data, data.settings_question), 6, data.context_record2, Near(data, data.settings_menu)) };
    if (choice == 0xffff) return;
    auto const result{ SettingsAction(ds, event, static_cast<uint16_t>(sp - OPTION_SP_DEPTH), choice - 1) };
    if (choice == 6 && result == 1) return;
  }
}

// Each service of the Settings routines is the direct call of the routine that provides it.
uint16_t svc_settings(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0, f;
  switch (static_cast<SettingsService>(id)) {
  case PRINT:   opeu_cursor_print(ds, { args, count }); break;
  case FORMAT:  opeu_request_format(ds, { args, count }); break;
  case DIALOG:  r = opeu_delay_dialog(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case NUMBER:  r = (uint8_t)opeu_choose_dialog(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case MESSAGE: opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case ASK:     r = opeu_confirm_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case LOCATION:
    f = b1_frame_enter();
    opeu_arrival_location(ds, svc_mappos, f, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2));
    port_frame_leave();
    break;
  case MUSIC_STOP:      opeu_music_stop(ds, svc_sound); break;
  case PRE_CONTEXT:     opeu_turn_pre_context(ds, svc_context); break;
  case SCREEN_RESET:    opeu_screen_reset(ds, (const DialogHost*)port_dialogs_host()); break;
  case ATTRIBUTE:       opeu_text_paper_set(ds, (TextHost*)port_textout_host(), (uint8_t)Arg(args, count, 0)); break;
  case DISK_RESET:      opeu_disk_reset(ds, svc_resopen); break;
  case FRAMED:          r = opeu_title_box(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case UI_TEXT:         opeu_ui_text(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case UI_WAIT:         opeu_delay_units(ds, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  case OPEN:            r = (uint16_t)opeu_open_resource(ds, svc_resopen, Arg(args, count, 0)); break;
  case MENU_CHOOSE:     r = opeu_list_dialog(ds, svc_listdlg, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case OUTLINE:         opeu_ui_outline(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case POSITION:        opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case TITLE:           opeu_army_title(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case YES_NO:          r = (uint16_t)opeu_prompt_yes_no(ds, svc_keyboard, Arg(args, count, 0)); break;
  case ACTIVE_BEGIN:    opeu_turn_active_begin(ds, (const DialogHost*)port_dialogs_host()); break;
  case PALETTE_TITLE:   opeu_palette_title_screen(ds, (const UiHost*)port_uiprim_host()); break;
  case PALETTE_DEFAULT: opeu_palette_daylight(ds, (const UiHost*)port_uiprim_host()); break;
  case FINISH_SIDE:     opeu_finish_side_initialization(ds, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case TURN_CONTEXT:    opeu_set_turn_context(ds, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case FREE_SEGMENTS:   opeu_free_segments(ds, svc_scenend); break;
  case MEMORY_RELEASE:  r = (uint16_t)opeu_memory_release(ds, svc_emsmap); break;
  case CLOSE_FILES:     opeu_close_data_files(ds, svc_scenend); break;
  case APPLICATION:     opeu_application_entry(ds, svc_app); break;
  case CREATE:          r = (uint16_t)opeu_dos_create((const char*)PORT_DS(Arg(args, count, 0))); break;
  case EXISTS:          r = (uint16_t)opeu_dos_exists((const char*)PORT_DS(Arg(args, count, 0))); break;
  case WRITE:           r = (uint16_t)opeu_dos_write((int16_t)Arg(args, count, 0), PORT_DS(Arg(args, count, 1)), Arg(args, count, 2)); break;
  case READ:            r = (uint16_t)opeu_dos_read((int16_t)Arg(args, count, 0), PORT_DS(Arg(args, count, 1)), Arg(args, count, 2)); break;
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
  case FAR_ACTION:  r = (uint16_t)port_call(ds, (uint32_t)Arg(args, count, 1) * 16u + Arg(args, count, 0), 0, 0); break;
  default:          port_unhandled("settings", id, count);
  }
  return r;
}

auto port_routine_opeu_turn_action_10(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  uint16_t sp = b1_sp_enter();
  (void)args;
  (void)count;
  opeu_turn_action_10(ds, svc_settings, sp);
  port_stack_leave(PORT_FRAME);
  return 0;
}
