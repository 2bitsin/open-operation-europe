// The recovered OPEN.EXE modules (recovered/) as they see each other and as
// the runner sees them. Each module calls the other modules' routines and the
// services (display, files, input, the FM driver) directly.
#pragma once

#include "open-data-segment.hpp"
#include "open.hpp"

#include <cstdint>
#include <span>

namespace opeu::intro {

// A 16-colour palette in DGROUP: three 4-bit levels per colour.
using Palette = std::span<std::uint8_t const, 48>;

namespace detail::scenes {
void RunOpening(Open& open);
}

namespace detail::helpers {
// Waits `ticks` of the effect countdown, whatever the input; 0 waits nothing.
void Delay(uint16_t ticks);
auto ResetMouse(Open& open) -> uint16_t;
void DrawPicture(Open& open, uint16_t x, uint16_t y, int32_t position);
void SetCurrentPalette(Open& open, Palette palette);
auto StepPalette(Open& open, Palette target) -> int16_t;
auto FadePalette(Open& open, Palette target, uint16_t mode) -> int16_t;
auto FadeOut(Open& open) -> int16_t;
auto FadeOutUninterrupted(Open& open) -> int16_t;
auto FadeToClosingPalette(Open& open) -> int16_t;
void SetIdentityPalette(Open& open);
auto PollInput() -> int16_t;
// Waits until the music timeline has run `ticks` further; 1 if a key or button came first.
auto WaitTicks(uint16_t ticks) -> int16_t;
auto OpenFileOrStop(Open& open, uint16_t name) -> int16_t;
// The stop after an allocation or open failure: the original spins in an
// empty loop; the host pumps its events instead, so a window close or a
// headless run's end still stops the program.
[[noreturn]] void Hang();
}

namespace detail::text_console {
void FlushKeys();
auto PutCharacter(Open& open, uint8_t c) -> uint8_t;
void PutString(Open& open, uint16_t s);
// cursor_print(format, ...): the words are pushed on the stack as the caller
// pushed them, the format's near pointer first.
void CursorPrint(Open& open, std::span<uint16_t const> words);
}

namespace detail::music {
void InitializeSound(Open& open);
void SendMusicControl(Open& open, uint8_t value);
auto IsMusicPlaying(Open& open) -> int;
void StartMusic(Open& open, uint8_t track);
// The FM driver's song start (song number, 1 = track 0) and stop (fade
// length), as the shutdown issues them without the music module's checks.
void StartSong(uint8_t song);
void StopMusic(uint8_t fade);
}

// The initialized data of the five DGROUP modules and of the song bank,
// which load-image.cpp places.
namespace detail::console_data {
extern uint8_t const   UNUSED_DIGIT_FONT[10][16];
extern uint32_t const  DRAW_HOOKS[4];
extern uint8_t const   TEXT_ERASE[8];
extern int16_t const   TEXT_X;
extern int16_t const   TEXT_Y;
extern int16_t const   TEXT_LEFT;
extern uint8_t const   TEXT_PAPER;
extern uint8_t const   TEXT_INK;
extern uint8_t const   TEXT_BACKGROUND;
extern int16_t const   TEXT_LARGE;
extern TextSaved const TEXT_SAVED;
extern uint8_t const   TEXT_LEAD;
extern int16_t const   TEXT_STATE;
extern uint16_t const  FORMAT_OUTPUT;
}

namespace detail::music_data {
extern uint8_t const MUSIC_CURRENT;
extern uint8_t const EFFECT_CURRENT;
extern char const    MUSIC_BANK[26];
extern char const    MUSIC_PATH[14];
}

namespace detail::scene_data {
extern char const     RESERVED_EMS_NAME[8];
extern int16_t const  RESERVED_SPRITE_MAPPED;
extern int16_t const  ABORTED;
extern int16_t const  FRAME_Y[11];
extern int16_t const  FRAME_H[11];
extern uint16_t const FRAME_OFF[11];
extern uint16_t const BOB_A[20];
extern int16_t const  BLAST_STEPS[6];
extern uint8_t const  BLAST_SLOTS[3];
extern int16_t const  BLAST_X[3];
extern int16_t const  BLAST_Y[3];
extern uint16_t const BLAST_MASKS[6];
extern uint16_t const BLAST_SPRITES[6];
extern uint8_t const  FLASH_SLOTS[3];
extern uint16_t const BOB_B[20];
extern int16_t const  FLAK_X[4];
extern int16_t const  FLAK_Y[4];
extern int16_t const  GUN_FLIPS[3];
extern int16_t const  SHELL_X[6];
extern int16_t const  SHELL_Y[6];
extern int16_t const  SHELL_W[6];
extern int16_t const  SHELL_WS[6];
extern int16_t const  SHELL_H[6];
extern uint16_t const SHELL_SPRITE[6];
extern uint16_t const SHELL_SAVE[6];
extern int16_t const  FLASH_ON;
extern int16_t const  FLASH_PHASE;
extern char const     ALLOC_ERR1[11];
extern char const     ALLOC_ERR2[11];
extern char const     ALLOC_ERR3[11];
extern char const     ALLOC_ERR4[11];
extern char const     ALLOC_ERR5[11];
extern char const     ALLOC_ERR6[11];
extern char const     ALLOC_ERR7[11];
extern char const     ALLOC_ERR9[11];
extern char const     CONSOLE_RESET[13];
extern char const     SCENARIO_FILE_NAME[11];
}

namespace detail::helper_data {
extern uint16_t const STREAM_COUNT;
extern uint8_t const  PALETTE[48];
extern uint8_t const  BLACK_PALETTE[48];
extern uint8_t const  SPARE_PALETTE[48];
extern uint8_t const  ICON[16][2];
extern char const     ERROR_FORMAT[16];
}

namespace detail::file_and_palette_data {
extern char const    LOGO_FILE_NAME[13];
extern char const    PICTURE_FILE_NAME[12];
extern char const    SPRITE_FILE_NAME[13];
extern int16_t const PICTURE_HANDLE;
extern int16_t const LOGO_HANDLE;
extern uint8_t const PALETTE_TABLE[112][48];
extern uint8_t const CLOSING_PALETTE[48];
}

// Far data: the FM song bank (recovered/data/song-bank.cpp). The namespace and the names are the probe's
// (probes/dump-open-bank), deferred to the tree-wide rename (#17).
namespace detail::ofmusic {
extern uint16_t const opeu_music_tracks[3][5];
extern uint8_t const  opeu_music_voices[447];
extern uint8_t const  opeu_music_events[8379];
}

using detail::helpers::Delay;
using detail::scenes::RunOpening;
using detail::music::SendMusicControl;
using detail::music::IsMusicPlaying;
using detail::music::StartMusic;
using detail::music::InitializeSound;
using detail::music::StartSong;
using detail::music::StopMusic;
using detail::text_console::CursorPrint;
using detail::text_console::FlushKeys;
using detail::text_console::PutCharacter;
using detail::text_console::PutString;
using detail::helpers::Hang;
using detail::helpers::ResetMouse;
using detail::helpers::FadeToClosingPalette;
using detail::helpers::FadeOut;
using detail::helpers::FadeOutUninterrupted;
using detail::helpers::OpenFileOrStop;
using detail::helpers::SetIdentityPalette;
using detail::helpers::PollInput;
using detail::helpers::WaitTicks;
using detail::helpers::FadePalette;
using detail::helpers::SetCurrentPalette;
using detail::helpers::StepPalette;
using detail::helpers::DrawPicture;

}
