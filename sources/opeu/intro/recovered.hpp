/* The recovered OPEN.EXE modules (recovered/) as the link layer (link/) and
 * the runner see them. Each module calls everything outside its own code
 * segment through one service callback of its own; the ids below are the
 * services, shared by the module and its link callback. */
#pragma once

#include "dgroup.hpp"

#include <cstdint>

namespace opeu::intro {

/* Microsoft C's <dos.h> REGS. */
struct WordRegs {
  uint16_t ax, bx, cx, dx, si, di, cflag;
};
struct ByteRegs {
  uint8_t al, ah, bl, bh, cl, ch, dl, dh;
};
union Regs {
  WordRegs x;
  ByteRegs h;
};

/* OPENMAIN (03ac) services: arguments in the original order, results in AX. */
enum class OpenEventId : uint16_t {
  SFX_PLAY,              /* (ticks) 0000:133e, int 66h F001h: counter A */
  SFX_BUSY,              /* () -> AX 0000:134b, counter A */
  FM_BUSY,               /* () -> AX 0000:1337, counter B (the music timeline) */
  INPUT_POLL,            /* () -> AX 062d:032e */
  VGA_CLEAR,             /* () 0000:0913 */
  PALETTE_FADE,          /* (mode, palette) -> AX 062d:0262 */
  PALETTE_STEP,          /* (palette) -> AX 062d:01fe */
  PALETTE_SET,           /* (palette) 062d:01bc */
  FADE_OUT,              /* () -> AX 062d:0304, abortable fade to black */
  FADE_OUT_WAIT,         /* () -> AX 062d:0310, uninterruptible */
  PICTURE_AT,            /* (x, y, position lo, hi) 062d:018e */
  GFX_WAIT,              /* () 032a:0048 */
  MUSIC_START,           /* (track) 0349:00a4 */
  MUSIC_COMMAND4,        /* (value) 0349:0020, MAIN's finish_side_initialization */
  DOS_SEEK,              /* (handle, lo, hi, origin) 0000:009b */
  DOS_READ_FAR,          /* (handle, segment, offset, n) 0000:0057 */
  DRAW_FAR,              /* (x, y, w, h, mode, segment, offset) 0000:0688 */
  BLIT_FAR,              /* (x, y, w, h, mode, segment, offset) 0000:044c */
  BITMAP_SAVE,           /* (x, y, w, h, segment, offset) 0000:1b56 */
  DISSOLVE,              /* (x, y, segment, offset, pattern) 0000:1e2f */
  SCREEN_COPY,           /* (x, y, w, h, to x, to y) 0000:1bb9 */
  UI_BOX,                /* (x, y, w, h, colour) 0000:0939 */
  FAR_COPY,              /* (source segment, source offset, segment, offset, n) 0000:1c2f */
  BUF_OR,                /* (segment, offset, w, h, source segment, source offset) 0000:1c74 */
  BUF_AND_MASK,          /* (segment, offset, w, h, mask segment, mask offset) 0000:1c94 */
  TIMELINE_START,        /* (ticks) 0000:132a, int 66h F101h: counter B */
  FM_NOOP,               /* () 0000:1352 */
  INSTALL_DOS_HANDLERS,  /* () 0000:1310 */
  DOS_ALLOC,             /* (paragraphs) -> AX 0000:0107 */
  DOS_FREE,              /* (segment) 0000:011a */
  DOS_CLOSE,             /* (handle) 0000:0032 */
  DOS_EXISTS,            /* (name) -> AX 0000:00d9 */
  STUB_09B7,             /* (1, 0) 0000:09b7 */
  INT86,                 /* (number, AX of the REGS block) 0000:1f40 */
  UI_TEXT,               /* (text) 02a0:071e */
  CURSOR_PRINT,          /* (format) 02a0:073e */
  MEMORY_RELEASE,        /* () 032a:005c */
  EMS_ALLOC_SPRITE,      /* () -> AX 032a:00d0 */
  EMS_DETECT,            /* () -> AX 032a:00f8 */
  SOUND_INITIALIZE,      /* () 0349:0002 */
  MUSIC_PLAYING,         /* () -> AX 0349:006e */
  MOUSE_RESET,           /* () 062d:0094 */
  FADE_ALT,              /* () 062d:031c, fade to palette 112 */
  IDENTITY_PALETTE,      /* () 062d:0328 */
  FILE_OR_DIE,           /* (name) -> AX 062d:0350 */
  HANG,                  /* () one turn of the stop after an allocation failure */
};

/* OUTIL (062d) services: results in DX:AX. */
enum class UtilServiceId : uint16_t {
  FM_BUSY,               /* () counter B */
  MOUSE_INT33,           /* int86(0x33): seven REGS words in and out */
  DOS_INT21,             /* int86(0x21) (number, AH) */
  DOS_READ_FAR,          /* (handle, segment, offset, n) */
  DOS_SEEK,              /* (handle, lo, hi, origin) */
  SET_PALETTE,           /* (first, count, palette) */
  IDENTITY_PALETTE,      /* () */
  UI_LINE,               /* (x1, y1, x2, y2, colour) */
  PICTURE_DECODE_MASKED, /* (x, y, reader offset, reader segment) */
  VGA_CLEAR,             /* () */
  TURN_INPUT_B,          /* () key waiting */
  INPUT_FLUSH,           /* () 02a0:0022 */
  CURSOR_PRINT,          /* (format, name) 02a0:073e */
  OPEN_ERROR_SPIN,       /* () one turn of the stop after an open error; nonzero ends it */
  TEXT_PUT,              /* (c) 02a0:04bc */
  OPEN_DELAY,            /* (ticks) 03ac:0004 */
  FAR_BYTE,              /* (segment, offset) */
  DOS_OPEN,              /* (name, mode) */
};

/* The picture decoder's reader: OUTIL's stream_file_byte, far 062d:0132. */
inline constexpr uint16_t STREAM_READER_SEGMENT{ 0x062d };
inline constexpr uint16_t STREAM_READER_OFFSET{ 0x0132 };

/* OTEXT (02a0) services. */
enum class TextEventId : uint16_t {
  KEY_READ,              /* () INT 16h AH=0 -> AX */
  KEY_WAITING,           /* () 0000:13d1 */
  STUB_A,                /* (value) 062d:0416 */
  BOX,                   /* (x, y, w, h, colour) 0000:0939 */
  CLEAR,                 /* () 0000:0913 */
  STUB_B,                /* (value) 062d:0418 */
  STUB_C,                /* (value) 062d:041a */
};

/* OEMSMAP (032a) calls of the EMM routines (segment 031c); pointer
 * arguments are DS offsets. */
enum class EmsEventId : uint16_t {
  PRESENT,               /* () */
  STATUS,                /* () */
  PAGE_COUNT,            /* (&count) */
  FREE_PAGES,            /* (&total, &unallocated) */
  FRAME,                 /* (&segment) */
  ALLOCATE,              /* (pages, &handle) */
  PAGE_ARRAY,            /* (pages) */
  MAP,                   /* (handle, physical, logical) */
  RELEASE,               /* (handle) */
};

/* OSOUND (0349) calls at the INT 66h boundary. */
enum class SoundServiceId : uint16_t {
  DRIVER_OPEN,           /* (bank offset, bank frame) 0000:13e0 */
  INT86,                 /* seven REGS words in and out */
  INT86X,                /* REGS, then es cs ss ds, in and out */
  SEGREAD,               /* es cs ss ds out */
};

/* The FM song bank the driver is opened with: far 0710:0002. */
inline constexpr uint16_t BANK_FRAME{ 0x0710 };
inline constexpr uint16_t BANK_OFFSET{ 0x0002 };

using OpenEvent = uint16_t (*)(uint8_t* ds, OpenEventId id, uint16_t const* args, uint16_t count);
using UtilService = uint32_t (*)(uint8_t* ds, UtilServiceId id, uint16_t* words, uint16_t count);
using TextEvent = uint16_t (*)(uint8_t* ds, TextEventId id, uint16_t const* args, uint16_t count);
using EmsEvent = uint16_t (*)(uint8_t* ds, EmsEventId id, uint16_t const* args, uint16_t count);
using SoundService = uint16_t (*)(uint8_t* ds, SoundServiceId call, uint16_t* words);

/* OTEXT's host: its event and the modeled stack pointer below which it
 * places its conversion buffer. */
struct TextHost {
  TextEvent event;
  uint16_t sp;
};

namespace detail::openmain {
void opeu_open_main(uint8_t* ds, OpenEvent event);
void opeu_open_delay(uint8_t* ds, OpenEvent event, uint16_t ticks);
}

namespace detail::outil {
auto opeu_mouse_reset(uint8_t* ds, UtilService event) -> uint16_t;
auto opeu_stream_file_byte(uint8_t* ds, UtilService event) -> uint8_t;
void opeu_picture_file_at(uint8_t* ds, UtilService event, uint16_t x, uint16_t y, int32_t position);
void opeu_open_palette_set(uint8_t* ds, UtilService event, uint8_t* palette);
auto opeu_open_palette_step(uint8_t* ds, UtilService event, uint8_t* target) -> int16_t;
auto opeu_open_palette_fade(uint8_t* ds, UtilService event, uint8_t* target, uint16_t mode) -> int16_t;
auto opeu_open_fade_in(uint8_t* ds, UtilService event) -> int16_t;
auto opeu_open_fade_in_wait(uint8_t* ds, UtilService event) -> int16_t;
auto opeu_open_fade_alt(uint8_t* ds, UtilService event) -> int16_t;
void opeu_open_identity_palette(uint8_t* ds, UtilService event);
auto opeu_open_input_poll(uint8_t* ds, UtilService event) -> int16_t;
auto opeu_open_file_or_die(uint8_t* ds, UtilService event, uint16_t name) -> int16_t;
void opeu_open_stub_a(uint8_t* ds, UtilService event);
void opeu_open_stub_b(uint8_t* ds, UtilService event);
void opeu_open_stub_c(uint8_t* ds, UtilService event);
}

namespace detail::otext {
void opeu_input_flush(uint8_t* ds, TextHost* host);
auto opeu_text_put(uint8_t* ds, TextHost* host, uint8_t c) -> uint8_t;
void opeu_ui_text(uint8_t* ds, TextHost* host, uint16_t s);
void opeu_cursor_print(uint8_t* ds, TextHost* host, uint16_t frame);
}

namespace detail::oemsmap {
void opeu_gfx_wait(uint8_t* ds, EmsEvent event);
auto opeu_memory_release(uint8_t* ds, EmsEvent event) -> int16_t;
auto opeu_ems_alloc_sprite(uint8_t* ds, EmsEvent event, uint16_t sp) -> int16_t;
auto opeu_ems_detect(uint8_t* ds, EmsEvent event, uint16_t sp) -> int16_t;
}

namespace detail::osound {
void opeu_sound_initialize(uint8_t* ds, SoundService service);
void opeu_finish_side_initialization(uint8_t* ds, SoundService service, uint8_t value);
auto opeu_music_playing(uint8_t* ds, SoundService service) -> int;
void opeu_music_start(uint8_t* ds, SoundService service, uint8_t track);
}

/* The initialized data of the six DGROUP modules and of the song bank,
 * which image.cpp places. */
namespace detail::odtext {
extern uint8_t opeu_open_digit_font[10][16];
extern uint32_t opeu_open_draw_hooks[4];
extern uint8_t opeu_text_erase[8];
extern uint8_t opeu_text_unused_014a[16];
extern int16_t opeu_text_x;
extern int16_t opeu_text_y;
extern int16_t opeu_text_left;
extern uint8_t opeu_text_paper;
extern uint8_t opeu_text_ink;
extern uint8_t opeu_text_background;
extern uint8_t opeu_text_z_0163;
extern int16_t opeu_text_large;
extern TextSaved opeu_text_saved;
extern uint8_t opeu_text_lead;
extern uint8_t opeu_text_z_0173;
extern int16_t opeu_text_state;
extern uint16_t opeu_format_output;
}

namespace detail::odems {
extern char opeu_ems_device_name[8];
extern int16_t opeu_sprite_mapped;
}

namespace detail::odsound {
extern uint8_t opeu_music_current;
extern uint8_t opeu_effect_current;
extern char opeu_music_bank[26];
extern char opeu_music_path[14];
}

namespace detail::odopen {
extern int16_t opeu_open_aborted;
extern int16_t opeu_open_frame_y[11];
extern int16_t opeu_open_frame_h[11];
extern uint16_t opeu_open_frame_off[11];
extern uint16_t opeu_open_bob_a[20];
extern int16_t opeu_open_blast_steps[6];
extern uint8_t opeu_open_blast_slots[3];
extern uint8_t opeu_open_z_0227;
extern int16_t opeu_open_blast_x[3];
extern int16_t opeu_open_blast_y[3];
extern uint16_t opeu_open_blast_masks[6];
extern uint16_t opeu_open_blast_sprites[6];
extern uint8_t opeu_open_flash_slots[3];
extern uint8_t opeu_open_z_024f;
extern uint16_t opeu_open_bob_b[20];
extern int16_t opeu_open_flak_x[4];
extern int16_t opeu_open_flak_y[4];
extern int16_t opeu_open_gun_flips[3];
extern int16_t opeu_open_shell_x[6];
extern int16_t opeu_open_shell_y[6];
extern int16_t opeu_open_shell_w[6];
extern int16_t opeu_open_shell_ws[6];
extern int16_t opeu_open_shell_h[6];
extern uint16_t opeu_open_shell_sprite[6];
extern uint16_t opeu_open_shell_save[6];
extern int16_t opeu_open_flash_on;
extern int16_t opeu_open_flash_phase;
extern char opeu_open_text_alloc_err1[11];
extern char opeu_open_text_alloc_err2[11];
extern char opeu_open_text_alloc_err3[11];
extern char opeu_open_text_alloc_err4[11];
extern char opeu_open_text_alloc_err5[11];
extern char opeu_open_text_alloc_err6[11];
extern char opeu_open_text_alloc_err7[11];
extern char opeu_open_text_alloc_err9[11];
extern char opeu_open_text_console[13];
extern char opeu_open_name_scesel[11];
}

namespace detail::odutil {
extern uint16_t opeu_stream_count;
extern uint8_t opeu_open_palette[48];
extern uint8_t opeu_open_black[48];
extern uint8_t opeu_open_spare_palette[48];
extern uint8_t opeu_open_icon[16][2];
extern char opeu_open_error_format[16];
}

namespace detail::odpal {
extern char opeu_open_name_fnt[13];
extern uint8_t opeu_open_z_0425;
extern char opeu_open_name_npk[12];
extern char opeu_open_name_mopengrp[13];
extern uint8_t opeu_open_z_043f;
extern int16_t opeu_picture_handle;
extern int16_t opeu_open_logo_file;
extern uint8_t opeu_open_palettes[112][48];
extern uint8_t opeu_open_fade_target[48];
}

/* Far data, frame 0710: the FM song bank from 0710:0002. */
namespace detail::ofmusic {
extern uint16_t opeu_music_tracks[3][5];
extern uint8_t opeu_music_voices[447];
extern uint8_t opeu_music_events[8379];
}

using detail::oemsmap::opeu_ems_alloc_sprite;
using detail::oemsmap::opeu_ems_detect;
using detail::oemsmap::opeu_gfx_wait;
using detail::oemsmap::opeu_memory_release;
using detail::openmain::opeu_open_delay;
using detail::openmain::opeu_open_main;
using detail::osound::opeu_finish_side_initialization;
using detail::osound::opeu_music_playing;
using detail::osound::opeu_music_start;
using detail::osound::opeu_sound_initialize;
using detail::otext::opeu_cursor_print;
using detail::otext::opeu_input_flush;
using detail::otext::opeu_text_put;
using detail::otext::opeu_ui_text;
using detail::outil::opeu_mouse_reset;
using detail::outil::opeu_open_fade_alt;
using detail::outil::opeu_open_fade_in;
using detail::outil::opeu_open_fade_in_wait;
using detail::outil::opeu_open_file_or_die;
using detail::outil::opeu_open_identity_palette;
using detail::outil::opeu_open_input_poll;
using detail::outil::opeu_open_palette_fade;
using detail::outil::opeu_open_palette_set;
using detail::outil::opeu_open_palette_step;
using detail::outil::opeu_open_stub_a;
using detail::outil::opeu_open_stub_b;
using detail::outil::opeu_open_stub_c;
using detail::outil::opeu_picture_file_at;
using detail::outil::opeu_stream_file_byte;

}
