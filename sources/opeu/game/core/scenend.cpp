// The end of a scenario: the outcome screen and the epilogue, and the shutdown helpers that free the memory segments
// and close the data files.
#include "core/scenend.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/dos-file.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/memory.hpp"
#include "core/msglook.hpp"
#include "core/people.hpp"
#include "core/settings.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/mouse.hpp"
#include "io/resopen.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "map/query.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/value.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

// The turn contexts (music) of the end screens.
constexpr std::uint8_t CONTEXT_DEFEAT{ 11 };
constexpr std::uint8_t CONTEXT_VICTORY{ 12 };
constexpr std::uint8_t CONTEXT_EPILOGUE{ 19 };

// SideRecord kind of a side's commander.
constexpr std::uint16_t COMMANDER{ 1 };

// The pictures of the outcome in SCEEND.DAT, by side.
constexpr std::int16_t VICTORY_PICTURE[]{ 0x1b, 0x93 };
constexpr std::int16_t DEFEAT_PICTURE[]{ 0x50, 0xd9 };

// The epilogue's message texts, page by page.
constexpr std::uint16_t EPILOGUE_PAGE_1[]{ 8, 9, 10, 11 };
constexpr std::uint16_t EPILOGUE_PAGE_2[]{ 0x34, 0x35, 0x36 };
constexpr std::uint16_t EPILOGUE_PAGE_3[]{ 0x4c, 0x4d, 0x4e, 0x4f };
constexpr std::uint16_t EPILOGUE_PAGE_4[]{ 4, 7, 0x37, 0x63 };

constexpr std::uint16_t MAX_SCORE{ 1000 };

// Prints the given message texts, one per line.
template <std::size_t N>
auto PrintMessages(Game& game, std::uint16_t const (&ids)[N]) -> void {
  for (auto const id : ids) opeu_cursor_print(game, std::array<std::uint16_t, 1>{ opeu_message_lookup(game, static_cast<int16_t>(id)) });
}

// Lets an epilogue page stand, then clears the screen to the dark palette for the next one.
auto NextPage(Game& game) -> void {
  opeu_delay_units(game, 0x68);
  opeu_delay_units(game, 0xb4);
  game.Machine().Display().Clear();
}

} // namespace

// The strength of the outcome side: the value of its ready units in its ready divisions, in hundreds.
uint16_t opeu_end_unit_strength(Game& game) {
  auto const& data{ Data(game) };
  uint16_t    value{ 0 };
  for (uint8_t g = 0; g < GROUPS_PER_SIDE; g++) {
    auto const group{ data.outcome_side * GROUPS_PER_SIDE + g };
    if (!(data.groups[group].flags & Bits(GroupFlag::READY))) continue;
    for (uint8_t slot = 0; slot < UNITS_PER_GROUP; slot++) {
      auto const& unit{ data.units[group * UNITS_PER_GROUP + slot] };
      if (unit.flags & Bits(UnitFlag::READY)) value += opeu_unit_value(game, Near(data, unit)) / 100;
    }
  }
  return value;
}

// The points of the objectives the outcome side holds.
uint16_t opeu_end_objective_points(Game& game) {
  auto const& data{ Data(game) };
  uint16_t    value{ 0 };
  for (auto const& objective : data.objectives)
    if (objective.active && static_cast<std::uint16_t>(opeu_objective_side(game, Near(data, objective))) == data.outcome_side) value += objective.strength_limit;
  return value;
}

// The dark palette of the epilogue pages.
void opeu_end_palette_dark(Game& game) {
  game.Machine().Display().LoadPalette(0, game.State().Bytes(offsetof(DataSegment, end_palette), 16 * RGB));
}

// The palette of the epilogue's last pictures.
void opeu_end_palette_show(Game& game) {
  game.Machine().Display().LoadPalette(0, game.State().Bytes(offsetof(DataSegment, end_palette_final), 16 * RGB));
}

// The outcome screen, the epilogue of a human winner, and the wait for the music to stop or a key; then the game
// quits. A victory by elimination (reason 0) or a defeat names the opposite side's commander; other victories rank
// the side by its score, min(strength + points, 1000), 100 more for reason 1. The routine's two text buffers, which services
// write, lie in its reserve.
void opeu_scenario_end(Game& game) {
  ServiceLocals const locals{ 0x1fc };
  auto&               data{ Data(game) };
  uint16_t const      pattern{ locals.At(0) };
  uint16_t const      caption{ locals.At(0xfa) };
  auto const          side{ data.outcome_side };
  auto const          reason{ data.outcome_reason };
  int16_t             picture;

  auto const copy_text{ [&](FarPointer const& text) { opeu_string_copy_far(game, pattern, port_far(text.segment, 0), text.offset); } };
  auto const format_caption{ [&](uint8_t record_side) {
    opeu_request_format(
        game, std::array<std::uint16_t, 5>{ caption, pattern, static_cast<std::uint16_t>(data.month), static_cast<std::uint16_t>(data.day), opeu_find_side_record(game, record_side, COMMANDER) });
  } };
  auto const opposite{ [&] { return opeu_opposite_side(side); } };

  if (data.side_status[side] == SIDE_PLAYED) {
    opeu_set_turn_context(game, CONTEXT_VICTORY);
    picture = VICTORY_PICTURE[side ? 1 : 0];
    if (reason == 0) {
      copy_text(data.end_victory_texts[0]);
      format_caption(opposite());
    } else {
      data.end_score  = opeu_end_unit_strength(game);
      data.end_score += opeu_end_objective_points(game);
      data.end_score  = opeu_min16(data.end_score, MAX_SCORE);
      if (reason == 1) data.end_score += 100;
      int16_t rank{ 0 };
      if (data.end_score >= 350) rank = 1;
      if (data.end_score >= 200 && data.end_score < 350) rank = 2;
      if (data.end_score < 200) rank = 3;
      copy_text(data.end_victory_texts[rank]);
      format_caption(side);
    }
  } else {
    opeu_set_turn_context(game, CONTEXT_DEFEAT);
    picture = DEFEAT_PICTURE[side ? 1 : 0];
    copy_text(data.end_defeat_texts[reason]);
    format_caption(opposite());
  }
  game.Machine().Display().Clear();
  opeu_palette_map_screen(game);
  auto handle{ static_cast<int16_t>(static_cast<uint16_t>(opeu_open_resource(game, offsetof(DataSegment, end_data_name)))) };
  opeu_picture_file(game, 80, 64, handle, picture);
  game.Machine().Files().Close(HandleOf(handle));
  opeu_army_position(game, 9, 336);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, end_text_format)), caption });
  opeu_finish_side_initialization(game, 1);
  opeu_delay_units(game, 0x93);
  if (data.side_control[side] == SIDE_PLAYED) {
    // The epilogue of endgrp.npk for a human winner: four pages of text over pictures, then the last two pictures.
    auto const picture_at{ [&](uint16_t x, uint16_t y, uint32_t at) {
      opeu_picture_file_at(game, x, y, handle, static_cast<std::uint32_t>((static_cast<uint16_t>(at))) | static_cast<std::uint32_t>((static_cast<uint16_t>(at >> 16))) << 16);
    } };
    opeu_set_turn_context(game, CONTEXT_EPILOGUE);
    opeu_finish_side_initialization(game, 0);
    handle = static_cast<int16_t>(static_cast<uint16_t>(opeu_open_resource(game, offsetof(DataSegment, end_picture_name))));
    game.Machine().Display().Clear();
    opeu_end_palette_dark(game);
    opeu_picture_file_masked(game, 0, 55, handle, 0);
    opeu_army_position(game, 12, 300);
    PrintMessages(game, EPILOGUE_PAGE_1);
    NextPage(game);
    opeu_end_palette_dark(game);
    picture_at(0, 85, 0x3d20);
    opeu_army_position(game, 12, 350);
    PrintMessages(game, EPILOGUE_PAGE_2);
    NextPage(game);
    opeu_end_palette_dark(game);
    picture_at(160, 35, 0xb4b1);
    opeu_army_position(game, 12, 300);
    PrintMessages(game, EPILOGUE_PAGE_3);
    NextPage(game);
    opeu_end_palette_dark(game);
    picture_at(160, 35, 0xd03b);
    opeu_army_position(game, 12, 280);
    PrintMessages(game, EPILOGUE_PAGE_4);
    opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, end_salute)), opeu_find_side_record(game, side, COMMANDER) });
    NextPage(game);
    opeu_end_palette_show(game);
    picture_at(0, 0, 0x10c78);
    opeu_delay_units(game, 0x1e);
    picture_at(128, 112, 0x13829);
    game.Machine().Files().Close(HandleOf(handle));
  }
  opeu_input_flush(game);
  opeu_delay_units(game, 0x1e);
  for (;;) {
    if (!static_cast<uint16_t>(opeu_music_playing(game)) || (data.input_mode ? static_cast<uint16_t>(opeu_turn_input_a(game)) : game.Machine().Input().KeyWaiting())) {
      opeu_quit_to_dos(game);
      return;
    }
  }
}

// Reports a segment that could not be freed: the screen is cleared, the name is not shown.
void opeu_free_error(Game& game, uint16_t name) {
  (void)name;
  game.Machine().Display().Clear();
  opeu_army_position(game, 0, 0);
}

// Frees the memory segments.
void opeu_free_segments(Game& game) {
  auto const& data{ Data(game) };
  auto const  release{ [&](uint16_t segment, auto const& name) {
    if (!static_cast<uint16_t>(opeu_dos_free(segment))) opeu_free_error(game, Near(data, name));
  } };
  release(data.overview_tile_segment, data.name_bmapseg);
  release(data.tile_segment, data.name_mmapseg);
  release(data.hex_display_segment, data.name_hdispseg);
  release(data.icon_segment, data.name_hunitseg);
  release(data.image_segment, data.name_grpseg);
  release(data.numdlg_save_segment, data.name_ms_winseg);
  release(data.numdlg_image_segment, data.name_ms_seg);
  release(data.sprite_segment, data.name_hfontseg);
  release(data.vseg_segment, data.name_vseg);
  release(data.portrait_segment, data.name_kaoseg);
  release(data.face_segment, data.name_armsseg);
  release(data.frame_segment, data.name_mdispseg);
}

// Closes the portrait, face and message-picture files.
void opeu_close_data_files(Game& game) {
  auto const& data{ Data(game) };
  game.Machine().Files().Close(HandleOf(data.portrait_handle));
  game.Machine().Files().Close(HandleOf(data.face_handle));
  game.Machine().Files().Close(HandleOf(data.message_picture_handle));
}

// The application's end of the game.
void opeu_app_finish(Game& game) {
  opeu_scenario_end(game);
}
