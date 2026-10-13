/* OPEN.EXE's data segment (DGROUP, paragraph 093A of its load image) as the
 * opening's code sees it: the initialized objects of the five data modules
 * (recovered/data, placed by image.cpp), then the KOEI library's and C
 * runtime's data and the BSS up to _end (DS:1C40). The stack (SS = DS)
 * follows above _end. */
#pragma once

#include <cstddef>
#include <cstdint>

namespace opeu::intro {

/* DS:0166, the console state in MAIN's order; OPEN never saves or restores it. */
struct TextSaved {
  int16_t x;
  int16_t y;
  int16_t left;
  uint8_t paper;
  uint8_t ink;
  uint8_t background;
  uint8_t z_016f;
  int16_t z_0170;
};

#pragma pack(push, 1)
struct OpenDataSegment {
  uint8_t library_data[0x92]; /* KOEI library and C runtime _DATA */
  /* ODTEXT */
  uint8_t   digit_font[10][16]; /* unreferenced 8x16 digit glyphs */
  uint32_t  draw_hooks[4];      /* far 0000:0680, 0000:069d, twice */
  uint8_t   text_erase[8];      /* the erase string of text_erase_char */
  uint8_t   text_unused_014a[16];
  int16_t   text_x;    /* column 0..79 */
  int16_t   text_y;    /* pixel row 0..399 */
  int16_t   text_left; /* left column (ESC R) */
  uint8_t   text_paper;
  uint8_t   text_ink;
  uint8_t   text_background;
  uint8_t   text_z_0163;
  int16_t   text_large; /* large mode (ESC Y 1) */
  TextSaved text_saved;
  uint8_t   text_lead; /* pending Shift-JIS lead byte */
  uint8_t   text_z_0173;
  int16_t   text_state;    /* escape-sequence state */
  uint16_t  format_output; /* request_format's destination */
  /* Reserved: kept for the layout until M4 */
  char    reserved_ems_name[8];
  int16_t reserved_sprite_mapped;
  /* ODSOUND */
  uint8_t music_current;
  uint8_t effect_current;
  char    music_bank[26];
  char    music_path[14];
  /* ODOPEN */
  int16_t  aborted;     /* a scene was aborted by input */
  int16_t  frame_y[11]; /* units scene: rows, heights, capture offsets */
  int16_t  frame_h[11];
  uint16_t frame_off[11];
  uint16_t bob_a[20];      /* bobbing curve of the first aircraft group */
  int16_t  blast_steps[6]; /* explosions */
  uint8_t  blast_slots[3];
  uint8_t  z_0227;
  int16_t  blast_x[3];
  int16_t  blast_y[3];
  uint16_t blast_masks[6];
  uint16_t blast_sprites[6];
  uint8_t  flash_slots[3]; /* flak flash palettes */
  uint8_t  z_024f;
  uint16_t bob_b[20]; /* bobbing curve of the second aircraft group */
  int16_t  flak_x[4];
  int16_t  flak_y[4];
  int16_t  gun_flips[3];
  int16_t  shell_x[6]; /* the six shell frames */
  int16_t  shell_y[6];
  int16_t  shell_w[6];
  int16_t  shell_ws[6];
  int16_t  shell_h[6];
  uint16_t shell_sprite[6];
  uint16_t shell_save[6];
  int16_t  flash_on; /* muzzle flash on/off and phase */
  int16_t  flash_phase;
  char     alloc_err1[11];
  char     alloc_err2[11];
  char     alloc_err3[11];
  char     alloc_err4[11];
  char     alloc_err5[11];
  char     alloc_err6[11];
  char     alloc_err7[11];
  char     alloc_err9[11];
  char     text_console[13]; /* the console reset sequence */
  char     name_scesel[11];
  /* ODUTIL */
  uint16_t stream_count; /* picture stream bytes left */
  uint8_t  palette[48];  /* the current palette */
  uint8_t  black[48];
  uint8_t  spare_palette[48];
  uint8_t  icon[16][2];
  char     error_format[16]; /* "OPEN ERROR=  %s" */
  /* ODPAL */
  char    name_fnt[13];
  uint8_t z_0425;
  char    name_npk[12];
  char    name_mopengrp[13];
  uint8_t z_043f;
  int16_t picture_handle; /* OPENGRP.NPK */
  int16_t logo_file;      /* KOEILOGO.FNT */
  uint8_t palettes[112][48];
  uint8_t fade_target[48];          /* palette 112 */
  uint8_t library_data_1974[0x284]; /* KOEI library and C runtime data */
  /* BSS */
  uint8_t  reserved_ems_pages[16]; /* kept for the layout until M4 */
  uint16_t reserved_sprite_handle; /* kept for the layout until M4 */
  uint8_t  library_bss_1c0a[4];
  uint16_t stream_offset; /* the picture stream's far pointer */
  uint16_t stream_segment;
  uint8_t  library_bss_1c12[14];
  uint16_t driver;   /* the music driver's open result */
  uint16_t buffer_a; /* segments of the 64 KB far buffers */
  uint16_t buffer_b;
  uint16_t buffer_c;
  uint16_t buffer_d;
  uint16_t buffer_e;
  uint16_t buffer_f;
  uint16_t sound_enabled;
  uint16_t buffer_g; /* the picture stream's buffer */
  uint16_t buffer_h; /* MOPENGRP.DAT: masks and shell sprites */
  uint8_t  library_bss_1c34[12];
};
#pragma pack(pop)

static_assert(offsetof(OpenDataSegment, digit_font) == 0x0092);
static_assert(offsetof(OpenDataSegment, draw_hooks) == 0x0132);
static_assert(offsetof(OpenDataSegment, text_erase) == 0x0142);
static_assert(offsetof(OpenDataSegment, text_unused_014a) == 0x014a);
static_assert(offsetof(OpenDataSegment, text_x) == 0x015a);
static_assert(offsetof(OpenDataSegment, text_y) == 0x015c);
static_assert(offsetof(OpenDataSegment, text_left) == 0x015e);
static_assert(offsetof(OpenDataSegment, text_paper) == 0x0160);
static_assert(offsetof(OpenDataSegment, text_ink) == 0x0161);
static_assert(offsetof(OpenDataSegment, text_background) == 0x0162);
static_assert(offsetof(OpenDataSegment, text_z_0163) == 0x0163);
static_assert(offsetof(OpenDataSegment, text_large) == 0x0164);
static_assert(offsetof(OpenDataSegment, text_saved) == 0x0166);
static_assert(offsetof(OpenDataSegment, text_lead) == 0x0172);
static_assert(offsetof(OpenDataSegment, text_z_0173) == 0x0173);
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
static_assert(offsetof(OpenDataSegment, z_0227) == 0x0227);
static_assert(offsetof(OpenDataSegment, blast_x) == 0x0228);
static_assert(offsetof(OpenDataSegment, blast_y) == 0x022e);
static_assert(offsetof(OpenDataSegment, blast_masks) == 0x0234);
static_assert(offsetof(OpenDataSegment, blast_sprites) == 0x0240);
static_assert(offsetof(OpenDataSegment, flash_slots) == 0x024c);
static_assert(offsetof(OpenDataSegment, z_024f) == 0x024f);
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
static_assert(offsetof(OpenDataSegment, text_console) == 0x033e);
static_assert(offsetof(OpenDataSegment, name_scesel) == 0x034b);
static_assert(offsetof(OpenDataSegment, stream_count) == 0x0356);
static_assert(offsetof(OpenDataSegment, palette) == 0x0358);
static_assert(offsetof(OpenDataSegment, black) == 0x0388);
static_assert(offsetof(OpenDataSegment, spare_palette) == 0x03b8);
static_assert(offsetof(OpenDataSegment, icon) == 0x03e8);
static_assert(offsetof(OpenDataSegment, error_format) == 0x0408);
static_assert(offsetof(OpenDataSegment, name_fnt) == 0x0418);
static_assert(offsetof(OpenDataSegment, z_0425) == 0x0425);
static_assert(offsetof(OpenDataSegment, name_npk) == 0x0426);
static_assert(offsetof(OpenDataSegment, name_mopengrp) == 0x0432);
static_assert(offsetof(OpenDataSegment, z_043f) == 0x043f);
static_assert(offsetof(OpenDataSegment, picture_handle) == 0x0440);
static_assert(offsetof(OpenDataSegment, logo_file) == 0x0442);
static_assert(offsetof(OpenDataSegment, palettes) == 0x0444);
static_assert(offsetof(OpenDataSegment, fade_target) == 0x1944);
static_assert(offsetof(OpenDataSegment, library_data_1974) == 0x1974);
static_assert(offsetof(OpenDataSegment, reserved_ems_pages) == 0x1bf8);
static_assert(offsetof(OpenDataSegment, reserved_sprite_handle) == 0x1c08);
static_assert(offsetof(OpenDataSegment, library_bss_1c0a) == 0x1c0a);
static_assert(offsetof(OpenDataSegment, stream_offset) == 0x1c0e);
static_assert(offsetof(OpenDataSegment, stream_segment) == 0x1c10);
static_assert(offsetof(OpenDataSegment, library_bss_1c12) == 0x1c12);
static_assert(offsetof(OpenDataSegment, driver) == 0x1c20);
static_assert(offsetof(OpenDataSegment, buffer_a) == 0x1c22);
static_assert(offsetof(OpenDataSegment, buffer_b) == 0x1c24);
static_assert(offsetof(OpenDataSegment, buffer_c) == 0x1c26);
static_assert(offsetof(OpenDataSegment, buffer_d) == 0x1c28);
static_assert(offsetof(OpenDataSegment, buffer_e) == 0x1c2a);
static_assert(offsetof(OpenDataSegment, buffer_f) == 0x1c2c);
static_assert(offsetof(OpenDataSegment, sound_enabled) == 0x1c2e);
static_assert(offsetof(OpenDataSegment, buffer_g) == 0x1c30);
static_assert(offsetof(OpenDataSegment, buffer_h) == 0x1c32);
static_assert(offsetof(OpenDataSegment, library_bss_1c34) == 0x1c34);
static_assert(sizeof(TextSaved) == 12);
static_assert(sizeof(OpenDataSegment) == 0x1c40); /* _end */

/* DGROUP at ds. */
// The segment is written through the result.
// NOLINTNEXTLINE(readability-non-const-parameter)
[[nodiscard]] inline auto Data(uint8_t* ds) -> OpenDataSegment& {
  return *reinterpret_cast<OpenDataSegment*>(ds);
}

/* The near pointer (DS offset) of a byte array in DGROUP (a string, a
 * palette); offsetof gives the others'. */
template <class T, std::size_t N>
  requires(alignof(T) == 1)
[[nodiscard]] inline auto Near(uint8_t const* ds, T const (&object)[N]) -> uint16_t {
  return static_cast<uint16_t>(reinterpret_cast<uint8_t const*>(object) - ds);
}

}
