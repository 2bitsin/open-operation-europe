// The end of a scenario: the outcome screen and the epilogue, the far-image accessors of the scenario copy, and the
// shutdown helpers that free the memory segments and close the data files.
#include "core/scenend.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
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
#include "link/glue.hpp"
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

// The scenario copy in the image segment starts with the scenario header.
constexpr std::uint16_t SCENARIO_HEADER{ offsetof(DataSegment, scenario_header) };

constexpr std::uint16_t MAX_SCORE{ 1000 };

// Prints the given message texts, one per line.
template <std::size_t N>
auto PrintMessages(Game& game, std::uint16_t const (&ids)[N]) -> void {
  for (auto const id : ids) opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(opeu_message_lookup(game, static_cast<int16_t>(id))) });
}

// Lets an epilogue page stand, then clears the screen to the dark palette for the next one.
auto NextPage(Game& game) -> void {
  opeu_delay_units(game, static_cast<uint8_t>(0x68));
  opeu_delay_units(game, static_cast<uint8_t>(0xb4));
  opeu_vga_clear();
}

} // namespace

// The byte, word and far pointer of a scenario object (a DS offset in the scenario) in the copy at the image segment.
uint8_t opeu_image_byte(Game const& game, const uint8_t image[65536], uint16_t p) {
  (void)game;
  return image[static_cast<uint16_t>(p - SCENARIO_HEADER)];
}

uint16_t opeu_image_word(Game const& game, const uint8_t image[65536], uint16_t p) {
  return static_cast<uint16_t>(opeu_image_byte(game, image, p) | opeu_image_byte(game, image, static_cast<uint16_t>(p + 1)) << 8);
}

uint32_t opeu_image_pointer(Game const& game, uint16_t p) {
  return static_cast<uint32_t>(Data(game).image_segment) << 16 | static_cast<uint16_t>(p - SCENARIO_HEADER);
}

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
  opeu_set_palette(static_cast<uint8_t>(0), static_cast<uint8_t>(16), game.State().Bytes(offsetof(DataSegment, end_palette)).data());
}

// The palette of the epilogue's last pictures.
void opeu_end_palette_show(Game& game) {
  opeu_set_palette(static_cast<uint8_t>(0), static_cast<uint8_t>(16), game.State().Bytes(offsetof(DataSegment, end_palette_final)).data());
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
    opeu_request_format(game, std::array<std::uint16_t, 5>{ static_cast<std::uint16_t>(caption), static_cast<std::uint16_t>(pattern), static_cast<std::uint16_t>(data.month),
                                                            static_cast<std::uint16_t>(data.day),
                                                            static_cast<std::uint16_t>(opeu_find_side_record(game, static_cast<uint8_t>(record_side), static_cast<uint8_t>(COMMANDER))) });
  } };
  auto const opposite{ [&] { return static_cast<uint8_t>(opeu_opposite_side(static_cast<uint8_t>(side))); } };

  if (data.side_status[side] == SIDE_PLAYED) {
    opeu_set_turn_context(game, svc_sound, static_cast<uint8_t>(CONTEXT_VICTORY));
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
    opeu_set_turn_context(game, svc_sound, static_cast<uint8_t>(CONTEXT_DEFEAT));
    picture = DEFEAT_PICTURE[side ? 1 : 0];
    copy_text(data.end_defeat_texts[reason]);
    format_caption(opposite());
  }
  opeu_vga_clear();
  opeu_palette_map_screen(game, static_cast<const UiHost*>(port_uiprim_host()));
  auto handle{ static_cast<int16_t>(static_cast<uint16_t>(opeu_open_resource(game, offsetof(DataSegment, end_data_name)))) };
  opeu_picture_file(game, static_cast<const UiHost*>(port_uiprim_host()), 80, 64, static_cast<int16_t>(handle), picture);
  opeu_dos_close(static_cast<int16_t>(handle));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(9), static_cast<int16_t>(336));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, end_text_format)), static_cast<std::uint16_t>(caption) });
  opeu_finish_side_initialization(game, svc_sound, static_cast<uint8_t>(1));
  opeu_delay_units(game, static_cast<uint8_t>(0x93));
  if (data.side_control[side] == SIDE_PLAYED) {
    // The epilogue of endgrp.npk for a human winner: four pages of text over pictures, then the last two pictures.
    auto const picture_at{ [&](uint16_t x, uint16_t y, uint32_t at) {
      opeu_picture_file_at(game, static_cast<const UiHost*>(port_uiprim_host()), x, y, static_cast<int16_t>(handle),
                           static_cast<int32_t>(static_cast<std::uint32_t>(static_cast<std::uint16_t>(static_cast<uint16_t>(at)))
                                                | static_cast<std::uint32_t>(static_cast<std::uint16_t>(static_cast<uint16_t>(at >> 16))) << 16));
    } };
    opeu_set_turn_context(game, svc_sound, static_cast<uint8_t>(CONTEXT_EPILOGUE));
    opeu_finish_side_initialization(game, svc_sound, static_cast<uint8_t>(0));
    handle = static_cast<int16_t>(static_cast<uint16_t>(opeu_open_resource(game, offsetof(DataSegment, end_picture_name))));
    opeu_vga_clear();
    opeu_end_palette_dark(game);
    opeu_picture_file_masked(game, static_cast<const UiHost*>(port_uiprim_host()), 0, 55, static_cast<int16_t>(handle), 0);
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(12), static_cast<int16_t>(300));
    PrintMessages(game, EPILOGUE_PAGE_1);
    NextPage(game);
    opeu_end_palette_dark(game);
    picture_at(0, 85, 0x3d20);
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(12), static_cast<int16_t>(350));
    PrintMessages(game, EPILOGUE_PAGE_2);
    NextPage(game);
    opeu_end_palette_dark(game);
    picture_at(160, 35, 0xb4b1);
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(12), static_cast<int16_t>(300));
    PrintMessages(game, EPILOGUE_PAGE_3);
    NextPage(game);
    opeu_end_palette_dark(game);
    picture_at(160, 35, 0xd03b);
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(12), static_cast<int16_t>(280));
    PrintMessages(game, EPILOGUE_PAGE_4);
    opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, end_salute)),
                                                          static_cast<std::uint16_t>(opeu_find_side_record(game, static_cast<uint8_t>(side), static_cast<uint8_t>(COMMANDER))) });
    NextPage(game);
    opeu_end_palette_show(game);
    picture_at(0, 0, 0x10c78);
    opeu_delay_units(game, static_cast<uint8_t>(0x1e));
    picture_at(128, 112, 0x13829);
    opeu_dos_close(static_cast<int16_t>(handle));
  }
  opeu_input_flush(game, static_cast<TextHost*>(port_textout_host()));
  opeu_delay_units(game, static_cast<uint8_t>(0x1e));
  for (;;) {
    if (!static_cast<uint16_t>(opeu_music_playing(game, svc_sound)) || (data.input_mode ? static_cast<uint16_t>(opeu_turn_input_a(game, svc_mouse)) : opeu_turn_input_b())) {
      opeu_quit_to_dos(game);
      return;
    }
  }
}

// Reports a segment that could not be freed: the screen is cleared, the name is not shown.
void opeu_free_error(Game& game, uint16_t name) {
  (void)name;
  opeu_vga_clear();
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0), static_cast<int16_t>(0));
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
  opeu_dos_close(static_cast<int16_t>(data.portrait_handle));
  opeu_dos_close(static_cast<int16_t>(data.face_handle));
  opeu_dos_close(static_cast<int16_t>(data.message_picture_handle));
}

// The application's end of the game.
void opeu_app_finish(Game& game) {
  opeu_scenario_end(game);
}
