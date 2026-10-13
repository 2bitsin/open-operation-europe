/* The recovered OPEN.EXE modules (recovered/) as they see each other and as
 * the runner sees them. Each module calls the other modules' routines and the
 * services (display, files, input, the FM driver) directly. */
#pragma once

#include "dgroup.hpp"
#include "open.hpp"

#include <cstdint>
#include <span>

namespace opeu::intro {

/* The FM song bank the driver is opened with: far 0710:0002. */
inline constexpr uint16_t BANK_FRAME{ 0x0710 };
inline constexpr uint16_t BANK_OFFSET{ 0x0002 };

/* A 16-colour palette in DGROUP: three 4-bit levels per colour. */
using Palette = std::span<std::uint8_t const, 48>;

namespace detail::openmain {
void opeu_open_main(Open& open);
void opeu_open_delay(uint16_t ticks);
}

namespace detail::outil {
auto opeu_mouse_reset(Open& open) -> uint16_t;
void opeu_picture_file_at(Open& open, uint16_t x, uint16_t y, int32_t position);
void opeu_open_palette_set(Open& open, Palette palette);
auto opeu_open_palette_step(Open& open, Palette target) -> int16_t;
auto opeu_open_palette_fade(Open& open, Palette target, uint16_t mode) -> int16_t;
auto opeu_open_fade_in(Open& open) -> int16_t;
auto opeu_open_fade_in_wait(Open& open) -> int16_t;
auto opeu_open_fade_alt(Open& open) -> int16_t;
void opeu_open_identity_palette(Open& open);
auto opeu_open_input_poll() -> int16_t;
auto opeu_open_file_or_die(Open& open, uint16_t name) -> int16_t;
void opeu_open_stub_a();
void opeu_open_stub_b();
void opeu_open_stub_c();
/* The stop after an allocation or open failure: the original spins in an
 * empty loop; the host pumps its events instead, so a window close or a
 * headless run's end still stops the program. */
[[noreturn]] void Hang();
}

namespace detail::otext {
void opeu_input_flush();
auto opeu_text_put(Open& open, uint8_t c) -> uint8_t;
void opeu_ui_text(Open& open, uint16_t s);
/* cursor_print(format, ...): the words are pushed on the stack as the caller
 * pushed them, the format's near pointer first. */
void CursorPrint(Open& open, std::span<uint16_t const> words);
}

namespace detail::osound {
void opeu_sound_initialize(Open& open);
void opeu_finish_side_initialization(Open& open, uint8_t value);
auto opeu_music_playing(Open& open) -> int;
void opeu_music_start(Open& open, uint8_t track);
/* The FM driver's song start (song number, 1 = track 0) and stop (fade
 * length), as the shutdown issues them without OSOUND's checks. */
void StartSong(uint8_t song);
void StopMusic(uint8_t fade);
}

/* The initialized data of the five DGROUP modules and of the song bank,
 * which image.cpp places. */
namespace detail::odtext {
extern uint8_t   opeu_open_digit_font[10][16];
extern uint32_t  opeu_open_draw_hooks[4];
extern uint8_t   opeu_text_erase[8];
extern uint8_t   opeu_text_unused_014a[16];
extern int16_t   opeu_text_x;
extern int16_t   opeu_text_y;
extern int16_t   opeu_text_left;
extern uint8_t   opeu_text_paper;
extern uint8_t   opeu_text_ink;
extern uint8_t   opeu_text_background;
extern uint8_t   opeu_text_z_0163;
extern int16_t   opeu_text_large;
extern TextSaved opeu_text_saved;
extern uint8_t   opeu_text_lead;
extern uint8_t   opeu_text_z_0173;
extern int16_t   opeu_text_state;
extern uint16_t  opeu_format_output;
}

namespace detail::odsound {
extern uint8_t opeu_music_current;
extern uint8_t opeu_effect_current;
extern char    opeu_music_bank[26];
extern char    opeu_music_path[14];
}

namespace detail::odopen {
extern char     opeu_reserved_ems_name[8];
extern int16_t  opeu_reserved_sprite_mapped;
extern int16_t  opeu_open_aborted;
extern int16_t  opeu_open_frame_y[11];
extern int16_t  opeu_open_frame_h[11];
extern uint16_t opeu_open_frame_off[11];
extern uint16_t opeu_open_bob_a[20];
extern int16_t  opeu_open_blast_steps[6];
extern uint8_t  opeu_open_blast_slots[3];
extern uint8_t  opeu_open_z_0227;
extern int16_t  opeu_open_blast_x[3];
extern int16_t  opeu_open_blast_y[3];
extern uint16_t opeu_open_blast_masks[6];
extern uint16_t opeu_open_blast_sprites[6];
extern uint8_t  opeu_open_flash_slots[3];
extern uint8_t  opeu_open_z_024f;
extern uint16_t opeu_open_bob_b[20];
extern int16_t  opeu_open_flak_x[4];
extern int16_t  opeu_open_flak_y[4];
extern int16_t  opeu_open_gun_flips[3];
extern int16_t  opeu_open_shell_x[6];
extern int16_t  opeu_open_shell_y[6];
extern int16_t  opeu_open_shell_w[6];
extern int16_t  opeu_open_shell_ws[6];
extern int16_t  opeu_open_shell_h[6];
extern uint16_t opeu_open_shell_sprite[6];
extern uint16_t opeu_open_shell_save[6];
extern int16_t  opeu_open_flash_on;
extern int16_t  opeu_open_flash_phase;
extern char     opeu_open_text_alloc_err1[11];
extern char     opeu_open_text_alloc_err2[11];
extern char     opeu_open_text_alloc_err3[11];
extern char     opeu_open_text_alloc_err4[11];
extern char     opeu_open_text_alloc_err5[11];
extern char     opeu_open_text_alloc_err6[11];
extern char     opeu_open_text_alloc_err7[11];
extern char     opeu_open_text_alloc_err9[11];
extern char     opeu_open_text_console[13];
extern char     opeu_open_name_scesel[11];
}

namespace detail::odutil {
extern uint16_t opeu_stream_count;
extern uint8_t  opeu_open_palette[48];
extern uint8_t  opeu_open_black[48];
extern uint8_t  opeu_open_spare_palette[48];
extern uint8_t  opeu_open_icon[16][2];
extern char     opeu_open_error_format[16];
}

namespace detail::odpal {
extern char    opeu_open_name_fnt[13];
extern uint8_t opeu_open_z_0425;
extern char    opeu_open_name_npk[12];
extern char    opeu_open_name_mopengrp[13];
extern uint8_t opeu_open_z_043f;
extern int16_t opeu_picture_handle;
extern int16_t opeu_open_logo_file;
extern uint8_t opeu_open_palettes[112][48];
extern uint8_t opeu_open_fade_target[48];
}

/* Far data, frame 0710: the FM song bank from 0710:0002. */
namespace detail::ofmusic {
extern uint16_t opeu_music_tracks[3][5];
extern uint8_t  opeu_music_voices[447];
extern uint8_t  opeu_music_events[8379];
}

using detail::openmain::opeu_open_delay;
using detail::openmain::opeu_open_main;
using detail::osound::opeu_finish_side_initialization;
using detail::osound::opeu_music_playing;
using detail::osound::opeu_music_start;
using detail::osound::opeu_sound_initialize;
using detail::osound::StartSong;
using detail::osound::StopMusic;
using detail::otext::CursorPrint;
using detail::otext::opeu_input_flush;
using detail::otext::opeu_text_put;
using detail::otext::opeu_ui_text;
using detail::outil::Hang;
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

}
