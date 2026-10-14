// OPEN.EXE's data segment (DGROUP) as the opening's code sees it: the initialized objects of the five data modules
// (recovered/data, placed by load-image.cpp), then the KOEI library's and C runtime's data and the BSS up to _end. The
// stack (SS = DS) follows above _end. The layout is the original's to the byte: past the song bank's track table the
// shutdown's song 5 reads this segment as music.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace opeu::intro::detail::open_data_segment {

// The console state in MAIN's order; OPEN never saves or restores it.
struct TextSaved {
  int16_t x;
  int16_t y;
  int16_t left;
  uint8_t paper;
  uint8_t ink;
  uint8_t background;
  uint8_t pad;
  int16_t large;
};

#pragma pack(push, 1)
struct OpenDataSegment {
  std::array<std::uint8_t, 0x92> library_data; // KOEI library and C runtime _DATA
  // The text console (recovered/data/console-data.hpp)
  std::array<std::array<std::uint8_t, 16>, 10> digit_font; // unreferenced 8x16 digit glyphs
  std::array<std::uint32_t, 4>                 draw_hooks; // the library's two draw routines, twice; never called through
  std::array<std::uint8_t, 8>                  text_erase; // the erase of one character; never printed
  std::array<std::uint8_t, 16>                 text_unused;
  int16_t                                      text_x;    // column 0..79
  int16_t                                      text_y;    // pixel row 0..399
  int16_t                                      text_left; // left column (ESC R)
  uint8_t                                      text_paper;
  uint8_t                                      text_ink;
  uint8_t                                      text_background;
  uint8_t                                      text_background_pad;
  int16_t                                      text_large; // large mode (ESC Y 1)
  TextSaved                                    text_saved;
  uint8_t                                      text_lead; // pending Shift-JIS lead byte
  uint8_t                                      text_lead_pad;
  int16_t                                      text_state;    // escape-sequence state
  uint16_t                                     format_output; // a formatted string's destination; always the console
  // Reserved: kept for the layout until M4
  std::array<char, 8> reserved_ems_name;
  int16_t             reserved_sprite_mapped;
  // The music (recovered/data/music-data.hpp)
  uint8_t              music_current;
  uint8_t              effect_current;
  std::array<char, 26> music_bank;
  std::array<char, 14> music_path;
  // The scenes (recovered/data/scene-data.hpp)
  int16_t                       aborted; // a scene was aborted by input
  std::array<std::int16_t, 11>  frame_y; // units scene: rows, heights, capture offsets
  std::array<std::int16_t, 11>  frame_h;
  std::array<std::uint16_t, 11> frame_off;
  std::array<std::uint16_t, 20> bob_a;       // bobbing curve of the first aircraft group
  std::array<std::int16_t, 6>   blast_steps; // explosions
  std::array<std::uint8_t, 3>   blast_slots;
  uint8_t                       blast_slots_pad;
  std::array<std::int16_t, 3>   blast_x;
  std::array<std::int16_t, 3>   blast_y;
  std::array<std::uint16_t, 6>  blast_masks;
  std::array<std::uint16_t, 6>  blast_sprites;
  std::array<std::uint8_t, 3>   flash_slots; // flak flash palettes
  uint8_t                       flash_slots_pad;
  std::array<std::uint16_t, 20> bob_b; // bobbing curve of the second aircraft group
  std::array<std::int16_t, 4>   flak_x;
  std::array<std::int16_t, 4>   flak_y;
  std::array<std::int16_t, 3>   gun_flips;
  std::array<std::int16_t, 6>   shell_x; // the six shell frames
  std::array<std::int16_t, 6>   shell_y;
  std::array<std::int16_t, 6>   shell_w;
  std::array<std::int16_t, 6>   shell_ws;
  std::array<std::int16_t, 6>   shell_h;
  std::array<std::uint16_t, 6>  shell_sprite;
  std::array<std::uint16_t, 6>  shell_save;
  int16_t                       flash_on; // muzzle flash on/off and phase
  int16_t                       flash_phase;
  std::array<char, 11>          alloc_err1;
  std::array<char, 11>          alloc_err2;
  std::array<char, 11>          alloc_err3;
  std::array<char, 11>          alloc_err4;
  std::array<char, 11>          alloc_err5;
  std::array<char, 11>          alloc_err6;
  std::array<char, 11>          alloc_err7;
  std::array<char, 11>          alloc_err9;
  std::array<char, 13>          console_reset; // the console reset sequence
  std::array<char, 11>          scenario_file_name;
  // The helpers (recovered/data/helper-data.hpp)
  uint16_t                                    stream_count; // picture stream bytes left
  std::array<std::uint8_t, 48>                palette;      // the current palette
  std::array<std::uint8_t, 48>                black;
  std::array<std::uint8_t, 48>                spare_palette;
  std::array<std::array<std::uint8_t, 2>, 16> icon;
  std::array<char, 16>                        error_format; // "OPEN ERROR=  %s"
  // The file names and the palette table (recovered/data/file-and-palette-data.hpp)
  std::array<char, 13>                          logo_file_name;
  uint8_t                                       logo_file_name_pad;
  std::array<char, 12>                          picture_file_name;
  std::array<char, 13>                          sprite_file_name;
  uint8_t                                       sprite_file_name_pad;
  int16_t                                       picture_handle; // OPENGRP.NPK
  int16_t                                       logo_handle;    // KOEILOGO.FNT
  std::array<std::array<std::uint8_t, 48>, 112> palettes;
  std::array<std::uint8_t, 48>                  closing_palette;             // palette 112
  std::array<std::uint8_t, 0x284>               library_data_after_palettes; // KOEI library and C runtime data
  // BSS
  std::array<std::uint8_t, 16> reserved_ems_pages;     // kept for the layout until M4
  uint16_t                     reserved_sprite_handle; // kept for the layout until M4
  std::array<std::uint8_t, 4>  library_bss_after_sprite_handle;
  uint16_t                     stream_offset; // the picture stream's far pointer
  uint16_t                     stream_segment;
  std::array<std::uint8_t, 14> library_bss_after_stream;
  uint16_t                     music_driver; // the music driver's open result
  uint16_t                     buffer_a;     // segments of the 64 KB far buffers
  uint16_t                     buffer_b;
  uint16_t                     buffer_c;
  uint16_t                     buffer_d;
  uint16_t                     buffer_e;
  uint16_t                     buffer_f;
  uint16_t                     sound_enabled;
  uint16_t                     stream_buffer; // the picture stream's buffer
  uint16_t                     sprite_sheet;  // MOPENGRP.DAT: masks and shell sprites
  std::array<std::uint8_t, 12> library_bss_after_buffers;
};
#pragma pack(pop)

static_assert(offsetof(OpenDataSegment, digit_font) == 0x0092);
static_assert(offsetof(OpenDataSegment, draw_hooks) == 0x0132);
static_assert(offsetof(OpenDataSegment, text_erase) == 0x0142);
static_assert(offsetof(OpenDataSegment, text_unused) == 0x014a);
static_assert(offsetof(OpenDataSegment, text_x) == 0x015a);
static_assert(offsetof(OpenDataSegment, text_y) == 0x015c);
static_assert(offsetof(OpenDataSegment, text_left) == 0x015e);
static_assert(offsetof(OpenDataSegment, text_paper) == 0x0160);
static_assert(offsetof(OpenDataSegment, text_ink) == 0x0161);
static_assert(offsetof(OpenDataSegment, text_background) == 0x0162);
static_assert(offsetof(OpenDataSegment, text_background_pad) == 0x0163);
static_assert(offsetof(OpenDataSegment, text_large) == 0x0164);
static_assert(offsetof(OpenDataSegment, text_saved) == 0x0166);
static_assert(offsetof(OpenDataSegment, text_lead) == 0x0172);
static_assert(offsetof(OpenDataSegment, text_lead_pad) == 0x0173);
static_assert(offsetof(OpenDataSegment, text_state) == 0x0174);
static_assert(offsetof(OpenDataSegment, format_output) == 0x0176);
static_assert(offsetof(OpenDataSegment, reserved_ems_name) == 0x0178);
static_assert(offsetof(OpenDataSegment, reserved_sprite_mapped) == 0x0180);
static_assert(offsetof(OpenDataSegment, music_current) == 0x0182);
static_assert(offsetof(OpenDataSegment, effect_current) == 0x0183);
static_assert(offsetof(OpenDataSegment, music_bank) == 0x0184);
static_assert(offsetof(OpenDataSegment, music_path) == 0x019e);
static_assert(offsetof(OpenDataSegment, aborted) == 0x01ac);
static_assert(offsetof(OpenDataSegment, frame_y) == 0x01ae);
static_assert(offsetof(OpenDataSegment, frame_h) == 0x01c4);
static_assert(offsetof(OpenDataSegment, frame_off) == 0x01da);
static_assert(offsetof(OpenDataSegment, bob_a) == 0x01f0);
static_assert(offsetof(OpenDataSegment, blast_steps) == 0x0218);
static_assert(offsetof(OpenDataSegment, blast_slots) == 0x0224);
static_assert(offsetof(OpenDataSegment, blast_slots_pad) == 0x0227);
static_assert(offsetof(OpenDataSegment, blast_x) == 0x0228);
static_assert(offsetof(OpenDataSegment, blast_y) == 0x022e);
static_assert(offsetof(OpenDataSegment, blast_masks) == 0x0234);
static_assert(offsetof(OpenDataSegment, blast_sprites) == 0x0240);
static_assert(offsetof(OpenDataSegment, flash_slots) == 0x024c);
static_assert(offsetof(OpenDataSegment, flash_slots_pad) == 0x024f);
static_assert(offsetof(OpenDataSegment, bob_b) == 0x0250);
static_assert(offsetof(OpenDataSegment, flak_x) == 0x0278);
static_assert(offsetof(OpenDataSegment, flak_y) == 0x0280);
static_assert(offsetof(OpenDataSegment, gun_flips) == 0x0288);
static_assert(offsetof(OpenDataSegment, shell_x) == 0x028e);
static_assert(offsetof(OpenDataSegment, shell_y) == 0x029a);
static_assert(offsetof(OpenDataSegment, shell_w) == 0x02a6);
static_assert(offsetof(OpenDataSegment, shell_ws) == 0x02b2);
static_assert(offsetof(OpenDataSegment, shell_h) == 0x02be);
static_assert(offsetof(OpenDataSegment, shell_sprite) == 0x02ca);
static_assert(offsetof(OpenDataSegment, shell_save) == 0x02d6);
static_assert(offsetof(OpenDataSegment, flash_on) == 0x02e2);
static_assert(offsetof(OpenDataSegment, flash_phase) == 0x02e4);
static_assert(offsetof(OpenDataSegment, alloc_err1) == 0x02e6);
static_assert(offsetof(OpenDataSegment, alloc_err2) == 0x02f1);
static_assert(offsetof(OpenDataSegment, alloc_err3) == 0x02fc);
static_assert(offsetof(OpenDataSegment, alloc_err4) == 0x0307);
static_assert(offsetof(OpenDataSegment, alloc_err5) == 0x0312);
static_assert(offsetof(OpenDataSegment, alloc_err6) == 0x031d);
static_assert(offsetof(OpenDataSegment, alloc_err7) == 0x0328);
static_assert(offsetof(OpenDataSegment, alloc_err9) == 0x0333);
static_assert(offsetof(OpenDataSegment, console_reset) == 0x033e);
static_assert(offsetof(OpenDataSegment, scenario_file_name) == 0x034b);
static_assert(offsetof(OpenDataSegment, stream_count) == 0x0356);
static_assert(offsetof(OpenDataSegment, palette) == 0x0358);
static_assert(offsetof(OpenDataSegment, black) == 0x0388);
static_assert(offsetof(OpenDataSegment, spare_palette) == 0x03b8);
static_assert(offsetof(OpenDataSegment, icon) == 0x03e8);
static_assert(offsetof(OpenDataSegment, error_format) == 0x0408);
static_assert(offsetof(OpenDataSegment, logo_file_name) == 0x0418);
static_assert(offsetof(OpenDataSegment, logo_file_name_pad) == 0x0425);
static_assert(offsetof(OpenDataSegment, picture_file_name) == 0x0426);
static_assert(offsetof(OpenDataSegment, sprite_file_name) == 0x0432);
static_assert(offsetof(OpenDataSegment, sprite_file_name_pad) == 0x043f);
static_assert(offsetof(OpenDataSegment, picture_handle) == 0x0440);
static_assert(offsetof(OpenDataSegment, logo_handle) == 0x0442);
static_assert(offsetof(OpenDataSegment, palettes) == 0x0444);
static_assert(offsetof(OpenDataSegment, closing_palette) == 0x1944);
static_assert(offsetof(OpenDataSegment, library_data_after_palettes) == 0x1974);
static_assert(offsetof(OpenDataSegment, reserved_ems_pages) == 0x1bf8);
static_assert(offsetof(OpenDataSegment, reserved_sprite_handle) == 0x1c08);
static_assert(offsetof(OpenDataSegment, library_bss_after_sprite_handle) == 0x1c0a);
static_assert(offsetof(OpenDataSegment, stream_offset) == 0x1c0e);
static_assert(offsetof(OpenDataSegment, stream_segment) == 0x1c10);
static_assert(offsetof(OpenDataSegment, library_bss_after_stream) == 0x1c12);
static_assert(offsetof(OpenDataSegment, music_driver) == 0x1c20);
static_assert(offsetof(OpenDataSegment, buffer_a) == 0x1c22);
static_assert(offsetof(OpenDataSegment, buffer_b) == 0x1c24);
static_assert(offsetof(OpenDataSegment, buffer_c) == 0x1c26);
static_assert(offsetof(OpenDataSegment, buffer_d) == 0x1c28);
static_assert(offsetof(OpenDataSegment, buffer_e) == 0x1c2a);
static_assert(offsetof(OpenDataSegment, buffer_f) == 0x1c2c);
static_assert(offsetof(OpenDataSegment, sound_enabled) == 0x1c2e);
static_assert(offsetof(OpenDataSegment, stream_buffer) == 0x1c30);
static_assert(offsetof(OpenDataSegment, sprite_sheet) == 0x1c32);
static_assert(offsetof(OpenDataSegment, library_bss_after_buffers) == 0x1c34);
static_assert(sizeof(TextSaved) == 12);
static_assert(sizeof(OpenDataSegment) == 0x1c40); // _end

} // namespace opeu::intro::detail::open_data_segment

namespace opeu::intro {
using detail::open_data_segment::OpenDataSegment;
using detail::open_data_segment::TextSaved;
} // namespace opeu::intro
