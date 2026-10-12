// The music and sound-effect driver interface (INT 66h). sound_driver holds the driver-open result, sound_enabled the
// player's switch, music_current and effect_current the track and effect playing (0xff initially), music_bank and
// music_path the bank and path the driver loads from. Music commands are AH=1 start, 2 stop, 3 status, 4 control,
// 5 load, 6 select, 7; effect commands are AH=10h load, 11h start (AL=effect+1), 12h stop, 13h status.
#include "io/sound.hpp"

#include "common/data-segment.hpp"
#include "common/host.hpp"
#include "glue.hpp"
#include "io/soundcue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// The service calls: the driver open (driver offset and segment, effects offset and segment), int86 (seven register
// words in and out), int86x (the registers, then four segment register words) and segread (four segment words out).
enum Service : std::uint16_t {
  DRIVER_OPEN = 0,
  INT86       = INT86_CALL,
  INT86X      = 2,
  SEGREAD     = 3,
};

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
auto CopyNear(std::uint8_t* ds, std::uint16_t source, std::uint16_t destination, std::uint16_t size) -> void {
  if (destination < source)
    for (std::uint16_t i{ 0 }; i < size; i++) ds[static_cast<std::uint16_t>(destination + i)] = ds[static_cast<std::uint16_t>(source + i)];
  else
    for (std::uint16_t i{ size }; i; i--) ds[static_cast<std::uint16_t>(destination + i - 1)] = ds[static_cast<std::uint16_t>(source + i - 1)];
}

// int86x: INT 66h with the registers and the segment registers; both come back updated.
auto Int86X(std::uint8_t* ds, HostService service, Regs& r, SegRegs* s) -> std::uint16_t {
  std::uint16_t words[11]{ r.x.ax, r.x.bx, r.x.cx, r.x.dx, r.x.si, r.x.di, r.x.cflag };
  words[7]  = s->es;
  words[8]  = s->cs;
  words[9]  = s->ss;
  words[10] = s->ds;
  auto const result{ service(ds, INT86X, words) };
  r.x = { words[0], words[1], words[2], words[3], words[4], words[5], words[6] };
  *s  = { words[7], words[8], words[9], words[10] };
  return result;
}

auto SegRead(std::uint8_t* ds, HostService service) -> SegRegs {
  std::uint16_t words[4]{};
  service(ds, SEGREAD, words);
  return { words[0], words[1], words[2], words[3] };
}

// A command with AL.
auto Command(std::uint8_t* ds, HostService service, std::uint8_t ah, std::uint8_t al = 0) -> std::uint16_t {
  Regs r{};
  r.h.ah = ah;
  r.h.al = al;
  return Int86(ds, service, r);
}
} // namespace

void opeu_sound_initialize(std::uint8_t* ds, HostService service) {
  auto&         data{ Data(ds) };
  std::uint16_t words[4]{ MUSIC_OFFSET, MUSIC_SEGMENT, EFFECTS_OFFSET, MUSIC_SEGMENT };
  data.sound_driver  = service(ds, DRIVER_OPEN, words);
  data.sound_enabled = data.sound_driver != 0;
}

void opeu_sound_reset(std::uint8_t* ds, HostService service) {
  opeu_sound_cue9(ds, service);
  opeu_effect_finish(ds, service);
}

void opeu_finish_side_initialization(std::uint8_t* ds, HostService service, std::uint8_t value) {
  auto const& data{ Data(ds) };
  if (data.sound_driver && data.sound_enabled) Command(ds, service, MUSIC_CONTROL, value);
}

void opeu_music_stop(std::uint8_t* ds, HostService service) {
  if (Data(ds).sound_driver) Command(ds, service, MUSIC_STOP, STOP_FADE);
}

auto opeu_music_playing(std::uint8_t* ds, HostService service) -> int {
  return (Command(ds, service, MUSIC_STATUS) & 0xff) != 0;
}

void opeu_music_start(std::uint8_t* ds, HostService service, std::uint8_t track) {
  auto& data{ Data(ds) };
  if (!data.sound_driver) return;
  if (opeu_music_playing(ds, service)) {
    if (data.music_current == track) return;
    opeu_music_stop(ds, service);
    while (opeu_music_playing(ds, service));
  }
  Command(ds, service, MUSIC_START, track);
  data.music_current = track;
}

void opeu_set_turn_context(std::uint8_t* ds, HostService service, std::uint8_t track) {
  if (Data(ds).sound_enabled) {
    opeu_music_start(ds, service, track);
    opeu_finish_side_initialization(ds, service, 1);
  }
}

// Copies the bank name (7 bytes) and the path (13 bytes) to music_bank and music_path and has the driver load them.
void opeu_music_load(std::uint8_t* ds, HostService service, std::uint16_t bank, std::uint16_t path) {
  auto& data{ Data(ds) };
  if (!data.sound_driver) return;
  CopyNear(ds, bank, Near(data, data.music_bank), 7);
  CopyNear(ds, path, Near(data, data.music_path), 13);
  auto segs{ SegRead(ds, service) };
  Regs r{};
  r.h.al = 0;
  r.x.dx = Near(data, data.music_bank);
  r.h.ah = MUSIC_LOAD;
  Int86X(ds, service, r, &segs);
}

void opeu_music_select(std::uint8_t* ds, HostService service, std::uint8_t value) {
  if (Data(ds).sound_driver) {
    Regs r{};
    r.h.al = 0;
    r.h.dl = value;
    r.h.ah = MUSIC_SELECT;
    Int86(ds, service, r);
  }
}

void opeu_music_load_select(std::uint8_t* ds, HostService service, std::uint16_t bank, std::uint16_t path, std::uint8_t value) {
  if (!Data(ds).sound_enabled) return;
  opeu_music_load(ds, service, bank, path);
  opeu_music_select(ds, service, value);
}

void opeu_music_command7(std::uint8_t* ds, HostService service) {
  Command(ds, service, MUSIC_7);
}

void opeu_effect_stop(std::uint8_t* ds, HostService service) {
  if (Data(ds).sound_driver) Command(ds, service, EFFECT_STOP);
}

auto opeu_effect_playing(std::uint8_t* ds, HostService service) -> int {
  if (!Data(ds).sound_driver) return 0;
  return (Command(ds, service, EFFECT_STATUS) & 0xff) != 0;
}

// Hands the driver the far effect data.
void opeu_effect_load(std::uint8_t* ds, HostService service, std::uint32_t data) {
  if (Data(ds).sound_driver) {
    Regs    r{};
    SegRegs segs{};
    r.h.ah  = EFFECT_LOAD;
    segs.ds = static_cast<std::uint16_t>(data >> 16);
    r.x.dx  = static_cast<std::uint16_t>(data);
    Int86X(ds, service, r, &segs);
  }
}

void opeu_effect_start(std::uint8_t* ds, HostService service, std::uint8_t effect) {
  auto& data{ Data(ds) };
  if (!data.sound_driver) return;
  if (opeu_effect_playing(ds, service)) {
    if (data.effect_current == effect) return;
    opeu_effect_stop(ds, service);
    while (opeu_effect_playing(ds, service));
  }
  Command(ds, service, EFFECT_START, static_cast<std::uint8_t>(effect + 1));
  data.effect_current = effect;
}

void opeu_effect_play(std::uint8_t* ds, HostService service, std::uint8_t effect) {
  if (Data(ds).sound_enabled) opeu_effect_start(ds, service, effect);
}

void opeu_effect_finish(std::uint8_t* ds, HostService service) {
  auto const& data{ Data(ds) };
  if (data.sound_driver && data.sound_enabled) {
    while (opeu_effect_playing(ds, service));
    opeu_effect_stop(ds, service);
  }
}

// Every int86 call goes to the native driver with the registers it reads and returns.
auto svc_sound(std::uint8_t*, std::uint16_t call, std::uint16_t* words) -> std::uint16_t {
  switch (call) {
  case DRIVER_OPEN: return static_cast<std::uint16_t>(opeu_sound_open(words[0], words[1], words[2], words[3]));
  case INT86:       {
    std::uint16_t ds_segment{ PORT_DS_SEGMENT };
    return port_sound_int66(words, &ds_segment);
  }
  case INT86X: return port_sound_int66(words, words + 10);
  case SEGREAD:
    words[0] = PORT_DS_SEGMENT;    // ES
    words[1] = SOUND_CODE_SEGMENT; // CS
    words[2] = PORT_DS_SEGMENT;    // SS
    words[3] = PORT_DS_SEGMENT;    // DS
    return 0;
  default: port_unhandled("sound", call, 0);
  }
  return 0;
}

auto port_routine_opeu_effect_finish(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_effect_finish(ds, svc_sound);
  return 0;
}

auto port_routine_opeu_finish_side_initialization(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_finish_side_initialization(ds, svc_sound, static_cast<std::uint8_t>(Arg(args, count, 0)));
  return 0;
}

auto port_routine_opeu_music_command7(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_music_command7(ds, svc_sound);
  return 0;
}

auto port_routine_opeu_music_stop(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_music_stop(ds, svc_sound);
  return 0;
}

auto port_routine_opeu_set_turn_context(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_set_turn_context(ds, svc_sound, static_cast<std::uint8_t>(Arg(args, count, 0)));
  return 0;
}
