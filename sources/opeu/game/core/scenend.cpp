// The end of a scenario: the outcome screen and the epilogue, the far-image accessors of the scenario copy, and the
// shutdown helpers that free the memory segments and close the data files.
#include "core/scenend.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/memory.hpp"
#include "core/msglook.hpp"
#include "core/people.hpp"
#include "core/settings.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "io/mouse.hpp"
#include "io/resopen.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "map/query.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/value.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the end of the scenario calls through its event.
enum Service : std::uint16_t {
  SET_TURN_CONTEXT = 0,  // set_turn_context(context)
  COPY_FAR         = 1,  // string_copy_far(destination, offset, segment)
  MIN16            = 2,  // min16(a, b)
  UNIT_VALUE       = 3,  // unit_value(unit)
  OBJECTIVE_SIDE   = 4,  // objective_side(objective)
  OPPOSITE_SIDE    = 5,  // opposite_side(side)
  SIDE_RECORD      = 6,  // find_side_record(side, kind)
  FORMAT           = 7,  // request_format(destination, format, month, day, name)
  VGA_CLEAR        = 8,  // vga_clear()
  MAP_PALETTE      = 9,  // palette_map_screen()
  OPEN             = 10, // open_resource(name)
  PICTURE          = 11, // picture_file(x, y, handle, block)
  PICTURE_MASKED   = 12, // picture_file_masked(x, y, handle, block)
  PICTURE_AT       = 13, // picture_file_at(x, y, handle, low, high)
  CLOSE            = 14, // dos_close(handle)
  POSITION         = 15, // army_position(x, y)
  PRINT            = 16, // army_print(format[, argument])
  FINISH_INIT      = 17, // finish_side_initialization(flag)
  DELAY            = 18, // delay_units(units)
  SET_PALETTE      = 19, // set_palette(first, count, rgb)
  LOOKUP           = 20, // message_lookup(id)
  FLUSH            = 21, // input_flush()
  MUSIC_PLAYING    = 22, // music_playing()
  INPUT_A          = 23, // turn_input_a()
  INPUT_B          = 24, // turn_input_b()
  QUIT             = 25, // quit_to_dos(): does not return
  FREE             = 26, // dos_free(segment)
};

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
auto PrintMessages(std::uint8_t* ds, HostEvent event, std::uint16_t const (&ids)[N]) -> void {
  for (auto const id : ids) Call(ds, event, PRINT, Call(ds, event, LOOKUP, id));
}

// Lets an epilogue page stand, then clears the screen to the dark palette for the next one.
auto NextPage(std::uint8_t* ds, HostEvent event) -> void {
  Call(ds, event, DELAY, 0x68);
  Call(ds, event, DELAY, 0xb4);
  Call(ds, event, VGA_CLEAR);
}

} // namespace

// The byte, word and far pointer of a scenario object (a DS offset in the scenario) in the copy at the image segment.
uint8_t opeu_image_byte(const uint8_t ds[65536], const uint8_t image[65536], uint16_t p) {
  (void)ds;
  return image[static_cast<uint16_t>(p - SCENARIO_HEADER)];
}

uint16_t opeu_image_word(const uint8_t ds[65536], const uint8_t image[65536], uint16_t p) {
  (void)ds;
  return Word(image, static_cast<uint16_t>(p - SCENARIO_HEADER));
}

uint32_t opeu_image_pointer(const uint8_t ds[65536], uint16_t p) {
  return static_cast<uint32_t>(Data(ds).image_segment) << 16 | static_cast<uint16_t>(p - SCENARIO_HEADER);
}

// The strength of the outcome side: the value of its ready units in its ready divisions, in hundreds.
uint16_t opeu_end_unit_strength(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  uint16_t    value{ 0 };
  for (uint8_t g = 0; g < GROUPS_PER_SIDE; g++) {
    auto const group{ data.outcome_side * GROUPS_PER_SIDE + g };
    if (!(data.groups[group].flags & Bits(GroupFlag::READY))) continue;
    for (uint8_t slot = 0; slot < UNITS_PER_GROUP; slot++) {
      auto const& unit{ data.units[group * UNITS_PER_GROUP + slot] };
      if (unit.flags & Bits(UnitFlag::READY)) value += Call(ds, event, UNIT_VALUE, Near(data, unit)) / 100;
    }
  }
  return value;
}

// The points of the objectives the outcome side holds.
uint16_t opeu_end_objective_points(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  uint16_t    value{ 0 };
  for (auto const& objective : data.objectives)
    if (objective.active && Call(ds, event, OBJECTIVE_SIDE, Near(data, objective)) == data.outcome_side) value += objective.strength_limit;
  return value;
}

// The dark palette of the epilogue pages.
void opeu_end_palette_dark(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, SET_PALETTE, 0, 16, offsetof(DataSegment, end_palette));
}

// The palette of the epilogue's last pictures.
void opeu_end_palette_show(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, SET_PALETTE, 0, 16, offsetof(DataSegment, end_palette_final));
}

// The outcome screen, the epilogue of a human winner, and the wait for the music to stop or a key; then the game
// quits. A victory by elimination (reason 0) or a defeat names the opposite side's commander; other victories rank
// the side by its score, min(strength + points, 1000), 100 more for reason 1. The routine's two text buffers, which services
// write, lie in its reserve.
void opeu_scenario_end(uint8_t ds[65536], HostEvent event) {
  ServiceLocals const locals{ 0x1fc };
  auto&               data{ Data(ds) };
  uint16_t const      pattern{ locals.At(0) };
  uint16_t const      caption{ locals.At(0xfa) };
  auto const          side{ data.outcome_side };
  auto const          reason{ data.outcome_reason };
  int16_t             picture;

  auto const copy_text{ [&](FarPointer const& text) { Call(ds, event, COPY_FAR, pattern, text.offset, text.segment); } };
  auto const format_caption{ [&](uint8_t record_side) { Call(ds, event, FORMAT, caption, pattern, data.month, data.day, Call(ds, event, SIDE_RECORD, record_side, COMMANDER)); } };
  auto const opposite{ [&] { return static_cast<uint8_t>(Call(ds, event, OPPOSITE_SIDE, side)); } };

  if (data.side_status[side] == SIDE_PLAYED) {
    Call(ds, event, SET_TURN_CONTEXT, CONTEXT_VICTORY);
    picture = VICTORY_PICTURE[side ? 1 : 0];
    if (reason == 0) {
      copy_text(data.end_victory_texts[0]);
      format_caption(opposite());
    } else {
      data.end_score  = opeu_end_unit_strength(ds, event);
      data.end_score += opeu_end_objective_points(ds, event);
      data.end_score  = Call(ds, event, MIN16, data.end_score, MAX_SCORE);
      if (reason == 1) data.end_score += 100;
      int16_t rank{ 0 };
      if (data.end_score >= 350) rank = 1;
      if (data.end_score >= 200 && data.end_score < 350) rank = 2;
      if (data.end_score < 200) rank = 3;
      copy_text(data.end_victory_texts[rank]);
      format_caption(side);
    }
  } else {
    Call(ds, event, SET_TURN_CONTEXT, CONTEXT_DEFEAT);
    picture = DEFEAT_PICTURE[side ? 1 : 0];
    copy_text(data.end_defeat_texts[reason]);
    format_caption(opposite());
  }
  Call(ds, event, VGA_CLEAR);
  Call(ds, event, MAP_PALETTE);
  auto handle{ static_cast<int16_t>(Call(ds, event, OPEN, offsetof(DataSegment, end_data_name))) };
  Call(ds, event, PICTURE, 80, 64, handle, picture);
  Call(ds, event, CLOSE, handle);
  Call(ds, event, POSITION, 9, 336);
  Call(ds, event, PRINT, offsetof(DataSegment, end_text_format), caption);
  Call(ds, event, FINISH_INIT, 1);
  Call(ds, event, DELAY, 0x93);
  if (data.side_control[side] == SIDE_PLAYED) {
    // The epilogue of endgrp.npk for a human winner: four pages of text over pictures, then the last two pictures.
    auto const picture_at{ [&](uint16_t x, uint16_t y, uint32_t at) { Call(ds, event, PICTURE_AT, x, y, handle, static_cast<uint16_t>(at), static_cast<uint16_t>(at >> 16)); } };
    Call(ds, event, SET_TURN_CONTEXT, CONTEXT_EPILOGUE);
    Call(ds, event, FINISH_INIT, 0);
    handle = static_cast<int16_t>(Call(ds, event, OPEN, offsetof(DataSegment, end_picture_name)));
    Call(ds, event, VGA_CLEAR);
    opeu_end_palette_dark(ds, event);
    Call(ds, event, PICTURE_MASKED, 0, 55, handle, 0);
    Call(ds, event, POSITION, 12, 300);
    PrintMessages(ds, event, EPILOGUE_PAGE_1);
    NextPage(ds, event);
    opeu_end_palette_dark(ds, event);
    picture_at(0, 85, 0x3d20);
    Call(ds, event, POSITION, 12, 350);
    PrintMessages(ds, event, EPILOGUE_PAGE_2);
    NextPage(ds, event);
    opeu_end_palette_dark(ds, event);
    picture_at(160, 35, 0xb4b1);
    Call(ds, event, POSITION, 12, 300);
    PrintMessages(ds, event, EPILOGUE_PAGE_3);
    NextPage(ds, event);
    opeu_end_palette_dark(ds, event);
    picture_at(160, 35, 0xd03b);
    Call(ds, event, POSITION, 12, 280);
    PrintMessages(ds, event, EPILOGUE_PAGE_4);
    Call(ds, event, PRINT, offsetof(DataSegment, end_salute), Call(ds, event, SIDE_RECORD, side, COMMANDER));
    NextPage(ds, event);
    opeu_end_palette_show(ds, event);
    picture_at(0, 0, 0x10c78);
    Call(ds, event, DELAY, 0x1e);
    picture_at(128, 112, 0x13829);
    Call(ds, event, CLOSE, handle);
  }
  Call(ds, event, FLUSH);
  Call(ds, event, DELAY, 0x1e);
  for (;;) {
    if (!Call(ds, event, MUSIC_PLAYING) || (data.input_mode ? Call(ds, event, INPUT_A) : Call(ds, event, INPUT_B))) {
      Call(ds, event, QUIT);
      return;
    }
  }
}

// Reports a segment that could not be freed: the screen is cleared, the name is not shown.
void opeu_free_error(uint8_t ds[65536], HostEvent event, uint16_t name) {
  (void)name;
  Call(ds, event, VGA_CLEAR);
  Call(ds, event, POSITION, 0, 0);
}

// Frees the DOS memory segments; segments mapped from EMS stay.
void opeu_free_segments(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  auto const  release{ [&](bool mapped, uint16_t segment, auto const& name) {
    if (!mapped && !Call(ds, event, FREE, segment)) opeu_free_error(ds, event, Near(data, name));
  } };
  release(false, data.overview_tile_segment, data.name_bmapseg);
  release(data.tile_mapped, data.tile_segment, data.name_mmapseg);
  release(false, data.hex_display_segment, data.name_hdispseg);
  release(false, data.icon_segment, data.name_hunitseg);
  release(false, data.image_segment, data.name_grpseg);
  release(false, data.numdlg_save_segment, data.name_ms_winseg);
  release(data.numdlg_image_mapped, data.numdlg_image_segment, data.name_ms_seg);
  release(data.sprite_mapped, data.sprite_segment, data.name_hfontseg);
  release(false, data.vseg_segment, data.name_vseg);
  release(false, data.portrait_segment, data.name_kaoseg);
  release(false, data.face_segment, data.name_armsseg);
  release(data.frame_mapped, data.frame_segment, data.name_mdispseg);
}

// Closes the portrait, face and message-picture files.
void opeu_close_data_files(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  Call(ds, event, CLOSE, data.portrait_handle);
  Call(ds, event, CLOSE, data.face_handle);
  Call(ds, event, CLOSE, data.message_picture_handle);
}

// The application's end of the game.
void opeu_app_finish(uint8_t ds[65536], HostEvent event) {
  opeu_scenario_end(ds, event);
}

// The services in the native build.
uint16_t svc_scenend(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case SET_TURN_CONTEXT: opeu_set_turn_context(ds, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case COPY_FAR:         r = opeu_string_copy_far(ds, Arg(args, count, 0), port_far(Arg(args, count, 2), 0), Arg(args, count, 1)); break;
  case MIN16:            r = opeu_min16(Arg(args, count, 0), Arg(args, count, 1)); break;
  case UNIT_VALUE:       r = opeu_unit_value(ds, Arg(args, count, 0)); break;
  case OBJECTIVE_SIDE:   r = opeu_objective_side(ds, Arg(args, count, 0)); break;
  case OPPOSITE_SIDE:    r = opeu_opposite_side((uint8_t)Arg(args, count, 0)); break;
  case SIDE_RECORD:      r = opeu_find_side_record(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case FORMAT:           opeu_request_format(ds, { args, count }); break;
  case VGA_CLEAR:        r = (uint16_t)opeu_vga_clear(); break;
  case MAP_PALETTE:      opeu_palette_map_screen(ds, (const UiHost*)port_uiprim_host()); break;
  case OPEN:             r = (uint16_t)opeu_open_resource(ds, svc_resopen, Arg(args, count, 0)); break;
  case PICTURE:          opeu_picture_file(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), (int16_t)Arg(args, count, 2), Arg(args, count, 3)); break;
  case PICTURE_MASKED:   opeu_picture_file_masked(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), (int16_t)Arg(args, count, 2), Arg(args, count, 3)); break;
  case PICTURE_AT:       opeu_picture_file_at(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), (int16_t)Arg(args, count, 2), (int32_t)Arg32(args, count, 3)); break;
  case CLOSE:            r = (uint16_t)opeu_dos_close((int16_t)Arg(args, count, 0)); break;
  case POSITION:         opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case PRINT:            opeu_cursor_print(ds, { args, count }); break;
  case FINISH_INIT:      opeu_finish_side_initialization(ds, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case DELAY:            opeu_delay_units(ds, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  case SET_PALETTE:      r = (uint16_t)opeu_set_palette((uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), PORT_DS(Arg(args, count, 2))); break;
  case LOOKUP:           r = opeu_message_lookup(ds, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  case FLUSH:            opeu_input_flush(ds, (TextHost*)port_textout_host()); break;
  case MUSIC_PLAYING:    r = (uint16_t)opeu_music_playing(ds, svc_sound); break;
  case INPUT_A:          r = (uint16_t)opeu_turn_input_a(ds, svc_mouse); break;
  case INPUT_B:          r = opeu_turn_input_b(); break;
  case QUIT:             opeu_quit_to_dos(ds, svc_settings); break;
  case FREE:             r = (uint16_t)opeu_dos_free(Arg(args, count, 0)); break;
  default:               port_unhandled("scenend", id, count);
  }
  return r;
}
