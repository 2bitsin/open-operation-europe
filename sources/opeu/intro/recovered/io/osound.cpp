/* OPEN 0349:0002..02b1 (image 0x3492..0x3742): the music interface to the
 * resident FM driver (INT 66h), the OPEN build of MAIN's SOUND (0d65). The
 * opening opens the driver with the song bank at 0710:0002 (OSYSLIB 13e0),
 * starts songs, asks whether one is playing, stops it and sends the control
 * command 4; each goes to the same driver instance MAIN uses afterwards, as
 * FMDRV stayed resident between the two programs. The rest of SOUND (song
 * banks loaded from files, the sound effects) is never called by the opening
 * and is not part of it. The routines that differ from MAIN's are marked
 * OPEN.
 */
#include "recovered.hpp"

#include "open-services.hpp"
#include "process.hpp"
#include "services.hpp"

#include <cstdint>

namespace opeu::intro::detail::osound {
namespace {
/* The driver's music functions, the high byte of the request. */
enum class Music : uint8_t {
  START   = 1,
  STOP    = 2,
  STATUS  = 3,
  CONTROL = 4,
};

/* One music request with its parameter byte; the driver's answer. The music
 * functions read neither BX, DX nor DS. */
auto Request(Music function, uint8_t value) -> uint16_t {
  auto     answer{ static_cast<uint16_t>(static_cast<unsigned>(function) << 8 | value) };
  uint16_t unused_b{ 0 };
  uint16_t unused_d{ 0 };
  uint16_t data_segment{ OPEN_DS_SEGMENT };
  port_fm_int66(&answer, &unused_b, &unused_d, &data_segment);
  return answer;
}
} // namespace

void StartSong(uint8_t song) {
  Request(Music::START, song);
}

void StopMusic(uint8_t fade) {
  Request(Music::STOP, fade);
}

/* OPEN: the driver is opened with the bank only. */
void InitializeSound(Open& open) {
  Data(open).music_driver  = static_cast<uint16_t>(OpenSongBank(BANK_OFFSET, OPEN_BANK_SEGMENT));
  Data(open).sound_enabled = static_cast<uint16_t>(Data(open).music_driver != 0);
}

void SendMusicControl(Open& open, uint8_t value) {
  if ((Data(open).music_driver != 0u) && (Data(open).sound_enabled != 0u)) Request(Music::CONTROL, value);
}

/* OPEN: no driver test. */
void FadeOutMusic() {
  StopMusic(20);
}

/* OPEN: tests the driver. */
auto IsMusicPlaying(Open& open) -> int {
  if (Data(open).music_driver == 0u) return 0;
  return static_cast<int>((Request(Music::STATUS, 0) & 0xff) != 0);
}

void StartMusic(Open& open, uint8_t track) {
  if (Data(open).music_driver == 0u) return;
  if (IsMusicPlaying(open) != 0) {
    if (Data(open).music_current == track) return;
    FadeOutMusic();
    while (IsMusicPlaying(open) != 0);
  }
  StartSong(static_cast<uint8_t>(track + 1));
  Data(open).music_current = track;
}

}
