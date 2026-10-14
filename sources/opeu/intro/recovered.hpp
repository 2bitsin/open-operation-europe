// The recovered OPEN.EXE modules (recovered/) as they see each other and as
// the runner sees them. Each module calls the other modules' routines and the
// services (display, files, input, the FM driver) directly.
#pragma once

#include "open-data-segment.hpp"
#include "open.hpp"
#include "recovered/data/song-bank.hpp" // the song bank the probe (probes/dump-open-bank) writes out

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
