// The recovered OPEN.EXE modules (recovered/) as they see each other and as
// the runner sees them. Each module calls the other modules' routines and the
// services (display, files, input, the FM driver) directly.
#pragma once

#include "dgroup.hpp"
#include "open.hpp"

#include <cstdint>
#include <span>

namespace opeu::intro {

// The FM song bank the driver is opened with: far 0710:0002.
inline constexpr uint16_t BANK_FRAME{ 0x0710 };
inline constexpr uint16_t BANK_OFFSET{ 0x0002 };

// A 16-colour palette in DGROUP: three 4-bit levels per colour.
using Palette = std::span<std::uint8_t const, 48>;

namespace detail::openmain {
void RunOpening(Open& open);
void Delay(uint16_t ticks);
}

namespace detail::outil {
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

namespace detail::otext {
void FlushKeys();
auto PutCharacter(Open& open, uint8_t c) -> uint8_t;
void PutString(Open& open, uint16_t s);
// cursor_print(format, ...): the words are pushed on the stack as the caller
// pushed them, the format's near pointer first.
void CursorPrint(Open& open, std::span<uint16_t const> words);
}

namespace detail::osound {
void InitializeSound(Open& open);
void SendMusicControl(Open& open, uint8_t value);
auto IsMusicPlaying(Open& open) -> int;
void StartMusic(Open& open, uint8_t track);
// The FM driver's song start (song number, 1 = track 0) and stop (fade
// length), as the shutdown issues them without OSOUND's checks.
void StartSong(uint8_t song);
void StopMusic(uint8_t fade);
}

// The initialized data of the five DGROUP modules and of the song bank,
// which image.cpp places.
namespace detail::odtext {
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

namespace detail::odsound {
extern uint8_t const MUSIC_CURRENT;
extern uint8_t const EFFECT_CURRENT;
extern char const    MUSIC_BANK[26];
extern char const    MUSIC_PATH[14];
}

namespace detail::odopen {
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

namespace detail::odutil {
extern uint16_t const STREAM_COUNT;
extern uint8_t const  PALETTE[48];
extern uint8_t const  BLACK_PALETTE[48];
extern uint8_t const  SPARE_PALETTE[48];
extern uint8_t const  ICON[16][2];
extern char const     ERROR_FORMAT[16];
}

namespace detail::odpal {
extern char const    LOGO_FILE_NAME[13];
extern char const    PICTURE_FILE_NAME[12];
extern char const    SPRITE_FILE_NAME[13];
extern int16_t const PICTURE_HANDLE;
extern int16_t const LOGO_HANDLE;
extern uint8_t const PALETTE_TABLE[112][48];
extern uint8_t const CLOSING_PALETTE[48];
}

// Far data, frame 0710: the FM song bank from 0710:0002.
namespace detail::ofmusic {
extern uint16_t const opeu_music_tracks[3][5];
extern uint8_t const  opeu_music_voices[447];
extern uint8_t const  opeu_music_events[8379];
}

using detail::openmain::Delay;
using detail::openmain::RunOpening;
using detail::osound::SendMusicControl;
using detail::osound::IsMusicPlaying;
using detail::osound::StartMusic;
using detail::osound::InitializeSound;
using detail::osound::StartSong;
using detail::osound::StopMusic;
using detail::otext::CursorPrint;
using detail::otext::FlushKeys;
using detail::otext::PutCharacter;
using detail::otext::PutString;
using detail::outil::Hang;
using detail::outil::ResetMouse;
using detail::outil::FadeToClosingPalette;
using detail::outil::FadeOut;
using detail::outil::FadeOutUninterrupted;
using detail::outil::OpenFileOrStop;
using detail::outil::SetIdentityPalette;
using detail::outil::PollInput;
using detail::outil::WaitTicks;
using detail::outil::FadePalette;
using detail::outil::SetCurrentPalette;
using detail::outil::StepPalette;
using detail::outil::DrawPicture;

}
