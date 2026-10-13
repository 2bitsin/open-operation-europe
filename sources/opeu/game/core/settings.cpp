// The Settings command: the battle, delay, map, music and picture options, saving the game, a new game and leaving
// to DOS.
#include "core/settings.hpp"
#include "common/dos-file.hpp"
#include "common/events.hpp"
#include "common/host.hpp"
#include "core/strings.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
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
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>
#include <iterator>
#include <utility>

using namespace opeu::game;

namespace {

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
auto Seek(Game& game, uint16_t handle, int32_t offset, uint16_t whence) -> uint16_t {
  return static_cast<uint16_t>(
      opeu_dos_seek(handle, static_cast<std::uint32_t>((static_cast<uint16_t>(offset))) | static_cast<std::uint32_t>((static_cast<uint16_t>(static_cast<uint32_t>(offset) >> 16))) << 16, whence));
}

// The save/quit menu's routines, by the enumerator save_quit_actions holds for the entry.
constexpr auto SAVE_QUIT_ACTIONS = DispatchRows<SaveQuitAction, auto (*)(Game&)->void>({
    { SaveQuitAction::SAVE_GAME, opeu_save_game         },
    { SaveQuitAction::NEW_GAME,  opeu_settings_new_game },
    { SaveQuitAction::LEAVE,     opeu_settings_leave    },
});

// A settings entry that reports nothing.
template <auto ROUTINE>
auto Unreported(Game& game) -> uint16_t {
  ROUTINE(game);
  return 0;
}

// The Settings menu's routines, by the enumerator settings_actions holds for the entry: only the save/quit menu's
// result is examined.
constexpr auto SETTINGS_ACTIONS = DispatchRows<SettingsAction, auto (*)(Game&)->uint16_t>({
    { SettingsAction::BATTLES,   Unreported<opeu_settings_battles>  },
    { SettingsAction::DELAY,     Unreported<opeu_settings_delay>    },
    { SettingsAction::MAP,       Unreported<opeu_settings_map>      },
    { SettingsAction::SOUND,     Unreported<opeu_settings_sound>    },
    { SettingsAction::PICTURES,  Unreported<opeu_settings_pictures> },
    { SettingsAction::SAVE_MENU, opeu_settings_save_menu            },
});

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
void opeu_settings_print(Game& game) {
  auto const& data{ Data(game) };
  auto const  battles{ static_cast<BattleView>(data.battle_view) };
  opeu_cursor_print(game, std::array<std::uint16_t, 6>{ Near(data, data.settings_summary), static_cast<std::uint16_t>(data.message_delay),
                                                        (data.sound_enabled == 1 && data.sound_driver ? Near(data, data.settings_music_on) : Near(data, data.settings_music_off)),
                                                        (battles == BattleView::ALL    ? Near(data, data.settings_battles_view)
                                                         : battles == BattleView::NONE ? Near(data, data.settings_battles_no)
                                                                                       : Near(data, data.settings_battles_select)),
                                                        (data.event_pictures == 1 ? Near(data, data.settings_pictures_on) : Near(data, data.settings_pictures_off)),
                                                        (!data.map_mode[0] ? Near(data, data.settings_aerial) : Near(data, data.settings_field)) });
}

// Which battles to watch: all, none or a choice before each.
void opeu_settings_battles(Game& game) {
  ServiceLocals const locals{ 102 }; // the message the format service writes
  auto&               data{ Data(game) };
  uint16_t const      message{ locals.At(0) };
  auto const          choice{ opeu_delay_dialog(game, Near(data, data.battles_question), 3, data.context_record2, Near(data, data.battles_menu)) };
  switch (choice) {
  case 1:  data.battle_view = std::to_underlying(BattleView::ALL); break;
  case 2:  data.battle_view = std::to_underlying(BattleView::NONE); break;
  case 3:  data.battle_view = std::to_underlying(BattleView::SELECT); break;
  default: return;
  }
  opeu_request_format(game, std::array<std::uint16_t, 3>{ message, Near(data, data.battles_reply), data.battles_descriptions[choice - 1] });
  opeu_control_message(game, message, data.context_record2);
}

// How long messages stay on screen.
void opeu_settings_delay(Game& game) {
  ServiceLocals const locals{ 100 }; // the message the format service writes
  auto&               data{ Data(game) };
  uint16_t const      message{ locals.At(0) };
  data.message_delay = (static_cast<uint8_t>(opeu_choose_dialog(game, Near(data, data.delay_question), 10, data.context_record2, 0)));
  opeu_request_format(game, std::array<std::uint16_t, 3>{ message, Near(data, data.delay_reply), static_cast<std::uint16_t>(data.message_delay) });
  opeu_control_message(game, message, data.context_record2);
}

// Switches between the aerial and the field map and shows the current unit's place on it.
void opeu_settings_map(Game& game) {
  ServiceLocals const locals{ 100 }; // the message the format service writes
  auto&               data{ Data(game) };
  uint16_t const      message{ locals.At(0) };
  data.map_mode[0]++;
  data.map_mode[0] &= 1;
  opeu_request_format(game, std::array<std::uint16_t, 3>{ message, Near(data, data.map_reply), (!data.map_mode[0] ? Near(data, data.map_aerial) : Near(data, data.map_field)) });
  opeu_control_message(game, message, data.context_record2);
  auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
  opeu_arrival_location(game, unit.x, unit.y, data.current_side);
}

// Turns the music off or on.
void opeu_settings_sound(Game& game) {
  ServiceLocals const locals{ 100 }; // the message the format service writes
  auto&               data{ Data(game) };
  uint16_t const      message{ locals.At(0) };
  if (data.sound_enabled == 1) {
    opeu_music_stop(game);
    data.sound_enabled = 0;
  } else {
    data.sound_enabled = 1;
    opeu_turn_pre_context(game);
  }
  opeu_request_format(game, std::array<std::uint16_t, 3>{ message, Near(data, data.sound_reply), (data.sound_enabled == 1 ? Near(data, data.sound_on) : Near(data, data.sound_off)) });
  opeu_control_message(game, message, data.context_record2);
}

// Turns the event pictures off or on.
void opeu_settings_pictures(Game& game) {
  ServiceLocals const locals{ 100 }; // the message the format service writes
  auto&               data{ Data(game) };
  uint16_t const      message{ locals.At(0) };
  data.event_pictures = data.event_pictures == 1 ? 0 : 1;
  opeu_request_format(game, std::array<std::uint16_t, 3>{ message, Near(data, data.pictures_reply), (data.event_pictures == 1 ? Near(data, data.pictures_will) : Near(data, data.pictures_will_not)) });
  opeu_control_message(game, message, data.context_record2);
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
void opeu_save_create(Game& game) {
  auto& data{ Data(game) };
  opeu_screen_reset(game);
  opeu_text_paper_set(game, 4);
  opeu_disk_reset(game);
  opeu_ui_text(game, opeu_title_box(game, 32, 188, Near(data, data.create_saving)));
  opeu_disk_reset(game);
  auto& first{ data.save_slots[0] };
  FillSlot(data, first);
  opeu_save_checksum(game, 0);
  auto const file{ static_cast<uint16_t>(opeu_dos_create(reinterpret_cast<const char*>(game.State().Bytes(Near(data, data.create_file_name)).data()))) };
  for (uint8_t slot = 0; slot < SAVE_FILE_SLOTS; slot++) {
    first.scenario = slot == 0 ? data.scenario : SLOT_EMPTY;
    opeu_dos_write(file, game.State().Bytes(SAVE_SLOTS_AT, SLOT_BYTES).data(), SLOT_BYTES);
  }
  opeu_save_unrebase(game);
  for (uint8_t slot = 0; slot < SAVE_FILE_SLOTS; slot++) opeu_dos_write(file, game.State().Bytes(SAVED_FIRST, SAVED_BYTES).data(), SAVED_BYTES);
  opeu_save_rebase(game);
  opeu_dos_close(file);
  if (static_cast<uint16_t>(opeu_dos_exists(reinterpret_cast<const char*>(game.State().Bytes(Near(data, data.create_check_name)).data())))) {
    opeu_ui_text(game, opeu_title_box(game, 31, 188, Near(data, data.create_saved)));
    opeu_delay_units(game, 15);
    opeu_text_paper_set(game, 0);
  }
}

// A framed prompt at text column x, pixel row y, with the cursor at its start; returns the text.
uint16_t opeu_save_prompt_box(Game& game, uint16_t x, uint16_t y, uint16_t text) {
  auto const width{ static_cast<uint16_t>(opeu_string_length(game, text) + 2) };
  opeu_ui_box(x - 1, y - 4, width, 24, 1);
  opeu_ui_outline(game, (x - 1) * 8, y - 4, (x + width) * 8 - 9, y + 19, 7);
  opeu_army_position(game, x, y);
  return text;
}

// Saves the game into a slot the player picks (creating the file on the first save).
void opeu_save_game(Game& game) {
  auto& data{ Data(game) };
  opeu_screen_reset(game);
  opeu_text_paper_set(game, 4);
  if (!static_cast<uint16_t>(opeu_dos_exists(reinterpret_cast<const char*>(game.State().Bytes(Near(data, data.save_check_name)).data())))) {
    opeu_save_create(game);
    if (!static_cast<uint16_t>(opeu_dos_exists(reinterpret_cast<const char*>(game.State().Bytes(Near(data, data.save_recheck_name)).data())))) {
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.save_failed_format), (opeu_title_box(game, 30, 188, Near(data, data.save_failed))) });
      opeu_delay_units(game, 20);
      opeu_text_paper_set(game, 0);
    }
  } else {
    opeu_text_paper_set(game, 0);
    auto const file{ static_cast<uint16_t>(opeu_open_resource(game, Near(data, data.save_open_name))) };
    for (uint8_t slot = 0; slot < SAVE_FILE_SLOTS; slot++) opeu_dos_read(file, game.State().Bytes(Near(data, data.save_slots[slot]), SLOT_BYTES).data(), SLOT_BYTES);
    for (;;) {
      opeu_text_paper_set(game, 4);
      opeu_ui_text(game, opeu_title_box(game, 36, 336, Near(data, data.save_where)));
      opeu_text_paper_set(game, 0);
      if (!opeu_list_dialog(game, Near(data, data.save_list), 1, 3)) {
        opeu_dos_close(file);
        break;
      }
      opeu_text_paper_set(game, 4);
      opeu_screen_copy(0, 240, 80, 48, 0, 320);
      opeu_ui_text(game, opeu_save_prompt_box(game, 35, 336, Near(data, data.save_confirm)));
      opeu_army_position(game, 39, 336);
      if (!static_cast<uint16_t>(opeu_prompt_yes_no(game, Near(data, data.save_answer)))) continue;
      opeu_ui_text(game, opeu_title_box(game, 35, 188, Near(data, data.save_saving)));
      auto& slot{ data.save_slots[data.save_slot] };
      FillSlot(data, slot);
      opeu_save_checksum(game, data.save_slot);
      Seek(game, file, static_cast<int32_t>(data.save_slot) * SLOT_BYTES, SEEK_FROM_START);
      opeu_dos_write(file, game.State().Bytes(Near(data, slot), SLOT_BYTES).data(), SLOT_BYTES);
      Seek(game, file, static_cast<int32_t>(SAVED_BYTES) * data.save_slot + SAVED_BLOCKS_AT, SEEK_FROM_START);
      opeu_save_unrebase(game);
      opeu_dos_write(file, game.State().Bytes(SAVED_FIRST, SAVED_BYTES).data(), SAVED_BYTES);
      opeu_save_rebase(game);
      opeu_dos_close(file);
      opeu_ui_text(game, opeu_title_box(game, 35, 188, Near(data, data.save_finished)));
      opeu_delay_units(game, 20);
      opeu_text_paper_set(game, 0);
      break;
    }
  }
  opeu_screen_reset(game);
  auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
  opeu_arrival_location(game, unit.x, unit.y, data.current_side);
  opeu_turn_active_begin(game);
}

// Shows the KOEI logo, shuts the sound and memory down and exits with code 100.
void opeu_quit_to_dos(Game& game) {
  auto const& data{ Data(game) };
  auto const  file{ static_cast<uint16_t>(opeu_open_resource(game, Near(data, data.logo_file_name))) };
  Seek(game, file, 0, SEEK_FROM_START);
  opeu_dos_read_far(file, data.bitmap_buffer, LOGO_AT, LOGO_BYTES);
  Seek(game, file, CAPTION_IN_FILE, SEEK_FROM_START);
  opeu_dos_read_far(file, data.bitmap_buffer, CAPTION_AT, CAPTION_BYTES);
  opeu_dos_close(file);
  opeu_vga_clear();
  opeu_palette_title_screen(game);
  opeu_finish_side_initialization(game, 0);
  opeu_gfx_blit(216, 120, 208, 112, 2, port_far(data.bitmap_buffer, LOGO_AT));
  opeu_gfx_blit(160, 232, 320, 40, 7, port_far(data.bitmap_buffer, CAPTION_AT));
  opeu_set_turn_context(game, 0);
  opeu_music_stop(game);
  opeu_fm_wait_start(200);
  while (opeu_fm_busy());
  opeu_sound_open(0, 0, 0, 0);
  opeu_fm_noop();
  opeu_text_mode();
  opeu_free_segments(game);
  opeu_disk_reset(game);
  port_exit(static_cast<int16_t>(QUIT_EXIT_CODE));
}

void opeu_settings_leave(Game& game) {
  auto const& data{ Data(game) };
  if (opeu_confirm_message(game, Near(data, data.leave_question), data.context_record2)) opeu_quit_to_dos(game);
}

// Starts a new game; the question is skipped when both sides are computer controlled.
void opeu_settings_new_game(Game& game) {
  auto& data{ Data(game) };
  if ((data.side_control[0] == SIDE_COMPUTER && data.side_control[1] == SIDE_COMPUTER) || opeu_confirm_message(game, Near(data, data.new_game_question), data.context_record2)) {
    opeu_set_turn_context(game, 0);
    opeu_music_stop(game);
    opeu_sound_open(0, 0, 0, 0);
    opeu_close_data_files(game);
    opeu_palette_daylight(game);
    opeu_fm_noop();
    opeu_text_mode();
    opeu_free_segments(game);
    opeu_vga_clear();
    opeu_screen_reset(game);
    data.startup_mode  = 0;
    data.daily_pending = 1;
    data.restart_flag  = 0;
    opeu_application_entry(game);
  }
}

// The save/quit menu; returns the choice, 0xffff when cancelled.
uint16_t opeu_settings_save_menu(Game& game) {
  auto const& data{ Data(game) };
  auto const  choice{ opeu_delay_dialog(game, Near(data, data.save_quit_question), 3, data.context_record2, Near(data, data.save_quit_menu)) };
  if (choice == 0xffff) return choice;
  Dispatch(SAVE_QUIT_ACTIONS, data.save_quit_actions.at(choice - 1u), game);
  return choice;
}

// The Settings command: the options menu until it is cancelled or the save/quit entry returns 1.
void opeu_turn_action_10(Game& game) {
  auto const& data{ Data(game) };
  for (;;) {
    opeu_army_title(game, Near(data, data.settings_title));
    opeu_settings_print(game);
    auto const choice{ opeu_delay_dialog(game, Near(data, data.settings_question), 6, data.context_record2, Near(data, data.settings_menu)) };
    if (choice == 0xffff) return;
    auto const result{ Dispatch(SETTINGS_ACTIONS, data.settings_actions.at(choice - 1u), game) };
    if (choice == 6 && result == 1) return;
  }
}
