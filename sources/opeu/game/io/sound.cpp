// The music and sound-effect driver interface (INT 66h). sound_driver holds the driver-open result, sound_enabled the
// player's switch, music_current and effect_current the track and effect playing (0xff initially), music_bank and
// music_path the bank and path the driver loads from. Music commands are AH=1 start, 2 stop, 3 status, 4 control,
// 5 load, 6 select, 7; effect commands are AH=10h load, 11h start (AL=effect+1), 12h stop, 13h status.
#include "io/sound.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/host.hpp"
#include "io/soundcue.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

enum DriverCommand : std::uint8_t {
  MUSIC_START   = 0x01,
  MUSIC_STOP    = 0x02,
  MUSIC_STATUS  = 0x03,
  MUSIC_CONTROL = 0x04,
  MUSIC_LOAD    = 0x05,
  MUSIC_SELECT  = 0x06,
  MUSIC_7       = 0x07,
  EFFECT_LOAD   = 0x10,
  EFFECT_START  = 0x11,
  EFFECT_STOP   = 0x12,
  EFFECT_STATUS = 0x13,
};

// The far data of the FM music and the effects (data/fmusic).
constexpr std::uint16_t MUSIC_SEGMENT{ 0x3d91 };
constexpr std::uint16_t MUSIC_OFFSET{ 0x0000 };
constexpr std::uint16_t EFFECTS_OFFSET{ 0x7bba };
constexpr std::uint8_t  STOP_FADE{ 20 };
constexpr std::uint16_t SOUND_CODE_SEGMENT{ 0x0d65 }; // CS as segread reports it

struct SegRegs {
  std::uint16_t es, cs, ss, ds;
};

// Copies `size` bytes between DS offsets: forward when the destination is below the source, else backward; both
// wrap within the segment.
auto CopyNear(Game& game, std::uint16_t source, std::uint16_t destination, std::uint16_t size) -> void {
  if (destination < source)
    for (std::uint16_t i{ 0 }; i < size; i++) Byte(game, static_cast<std::uint16_t>(destination + i)) = Byte(game, static_cast<std::uint16_t>(source + i));
  else
    for (std::uint16_t i{ size }; i; i--) Byte(game, static_cast<std::uint16_t>(destination + i - 1)) = Byte(game, static_cast<std::uint16_t>(source + i - 1));
}

// int86: INT 66h with the registers, DS the data segment.
auto SoundInterrupt(std::uint16_t* words) -> std::uint16_t {
  std::uint16_t ds_segment{ PORT_DS_SEGMENT };
  return port_sound_int66(words, &ds_segment);
}

// int86x: INT 66h with the registers and the segment registers; both come back updated.
auto Int86X(Game& game, Regs& r, SegRegs* s) -> std::uint16_t {
  std::uint16_t words[11]{ r.x.ax, r.x.bx, r.x.cx, r.x.dx, r.x.si, r.x.di, r.x.cflag };
  words[7]  = s->es;
  words[8]  = s->cs;
  words[9]  = s->ss;
  words[10] = s->ds;
  auto const result{ port_sound_int66(words, words + 10) };
  r.x = { words[0], words[1], words[2], words[3], words[4], words[5], words[6] };
  *s  = { words[7], words[8], words[9], words[10] };
  return result;
}

// The segment registers: ES, SS and DS the data segment, CS the sound code.
auto SegRead(Game& game) -> SegRegs {
  return { PORT_DS_SEGMENT, SOUND_CODE_SEGMENT, PORT_DS_SEGMENT, PORT_DS_SEGMENT };
}

// A command with AL.
auto Command(Game& game, std::uint8_t ah, std::uint8_t al = 0) -> std::uint16_t {
  Regs r{};
  r.h.ah = ah;
  r.h.al = al;
  return Int86(r, SoundInterrupt);
}
} // namespace

void opeu_sound_initialize(Game& game) {
  auto&         data{ Data(game) };
  std::uint16_t words[4]{ MUSIC_OFFSET, MUSIC_SEGMENT, EFFECTS_OFFSET, MUSIC_SEGMENT };
  data.sound_driver  = static_cast<std::uint16_t>(opeu_sound_open(words[0], words[1], words[2], words[3]));
  data.sound_enabled = data.sound_driver != 0;
}

void opeu_sound_reset(Game& game) {
  opeu_sound_cue9(game);
  opeu_effect_finish(game);
}

void opeu_finish_side_initialization(Game& game, std::uint8_t value) {
  auto const& data{ Data(game) };
  if (data.sound_driver && data.sound_enabled) Command(game, MUSIC_CONTROL, value);
}

void opeu_music_stop(Game& game) {
  if (Data(game).sound_driver) Command(game, MUSIC_STOP, STOP_FADE);
}

auto opeu_music_playing(Game& game) -> int {
  return (Command(game, MUSIC_STATUS) & 0xff) != 0;
}

void opeu_music_start(Game& game, std::uint8_t track) {
  auto& data{ Data(game) };
  if (!data.sound_driver) return;
  if (opeu_music_playing(game)) {
    if (data.music_current == track) return;
    opeu_music_stop(game);
    while (opeu_music_playing(game));
  }
  Command(game, MUSIC_START, track);
  data.music_current = track;
}

void opeu_set_turn_context(Game& game, std::uint8_t track) {
  if (Data(game).sound_enabled) {
    opeu_music_start(game, track);
    opeu_finish_side_initialization(game, 1);
  }
}

// Copies the bank name (7 bytes) and the path (13 bytes) to music_bank and music_path and has the driver load them.
void opeu_music_load(Game& game, std::uint16_t bank, std::uint16_t path) {
  auto& data{ Data(game) };
  if (!data.sound_driver) return;
  CopyNear(game, bank, Near(data, data.music_bank), 7);
  CopyNear(game, path, Near(data, data.music_path), 13);
  auto segs{ SegRead(game) };
  Regs r{};
  r.h.al = 0;
  r.x.dx = Near(data, data.music_bank);
  r.h.ah = MUSIC_LOAD;
  Int86X(game, r, &segs);
}

void opeu_music_select(Game& game, std::uint8_t value) {
  if (Data(game).sound_driver) {
    Regs r{};
    r.h.al = 0;
    r.h.dl = value;
    r.h.ah = MUSIC_SELECT;
    Int86(r, SoundInterrupt);
  }
}

void opeu_music_load_select(Game& game, std::uint16_t bank, std::uint16_t path, std::uint8_t value) {
  if (!Data(game).sound_enabled) return;
  opeu_music_load(game, bank, path);
  opeu_music_select(game, value);
}

void opeu_music_command7(Game& game) {
  Command(game, MUSIC_7);
}

void opeu_effect_stop(Game& game) {
  if (Data(game).sound_driver) Command(game, EFFECT_STOP);
}

auto opeu_effect_playing(Game& game) -> int {
  if (!Data(game).sound_driver) return 0;
  return (Command(game, EFFECT_STATUS) & 0xff) != 0;
}

// Hands the driver the far effect data.
void opeu_effect_load(Game& game, std::uint32_t data) {
  if (Data(game).sound_driver) {
    Regs    r{};
    SegRegs segs{};
    r.h.ah  = EFFECT_LOAD;
    segs.ds = static_cast<std::uint16_t>(data >> 16);
    r.x.dx  = static_cast<std::uint16_t>(data);
    Int86X(game, r, &segs);
  }
}

void opeu_effect_start(Game& game, std::uint8_t effect) {
  auto& data{ Data(game) };
  if (!data.sound_driver) return;
  if (opeu_effect_playing(game)) {
    if (data.effect_current == effect) return;
    opeu_effect_stop(game);
    while (opeu_effect_playing(game));
  }
  Command(game, EFFECT_START, static_cast<std::uint8_t>(effect + 1));
  data.effect_current = effect;
}

void opeu_effect_play(Game& game, std::uint8_t effect) {
  if (Data(game).sound_enabled) opeu_effect_start(game, effect);
}

void opeu_effect_finish(Game& game) {
  auto const& data{ Data(game) };
  if (data.sound_driver && data.sound_enabled) {
    while (opeu_effect_playing(game));
    opeu_effect_stop(game);
  }
}
