/* OPEN 0349:0002..02b1 (image 0x3492..0x3742): INT 66h music and sound-effect
 * driver interface, the OPEN build of MAIN's SOUND (0d65), whose C text is
 * copied here; the routines that differ are marked OPEN. Music commands are
 * AH=1 start, 2 stop, 3 status, 4 control, 5 load, 6 select, 7; effect
 * commands are AH=10h load, 11h start, 12h stop, 13h status. OPEN has no
 * sound reset or effect finish; the driver is opened with the FM driver image
 * at 0710:0002 only (0000:13e0).
 *
 * ds is the original 64 KiB data segment; near pointers are DS offsets and
 * far pointers are segment:offset values relative to the load segment. The
 * service is the INT 66h boundary (SoundServiceId). The original passes the
 * same REGS as input and output; unassigned fields are zero here.
 */
#include "recovered.hpp"

namespace opeu::intro::detail::osound {
namespace {
struct SegRegs {
  uint16_t es, cs, ss, ds;
};

auto host_open(uint8_t* ds, SoundService service) -> uint16_t {
  uint16_t words[2] = { BANK_OFFSET, BANK_FRAME };
  return service(ds, SoundServiceId::DRIVER_OPEN, words);
}

/* 09cc:000a copies forward when destination < source, else backward;
 * both near pointers wrap within DS. */
auto host_copy(uint8_t* ds, uint16_t source, uint16_t destination, uint16_t size) -> void {
  uint16_t i = 0;
  if (destination < source)
    for (i = 0; i < size; i++) ds[static_cast<uint16_t>(destination + i)] = ds[static_cast<uint16_t>(source + i)];
  else
    for (i = size; i != 0u; i--) ds[static_cast<uint16_t>(destination + i - 1)] = ds[static_cast<uint16_t>(source + i - 1)];
}

auto host_int86(uint8_t* ds, SoundService service, Regs* r, SegRegs* s) -> uint16_t {
  uint16_t words[11];
  uint16_t result = 0;
  words[0]        = r->x.ax;
  words[1]        = r->x.bx;
  words[2]        = r->x.cx;
  words[3]        = r->x.dx;
  words[4]        = r->x.si;
  words[5]        = r->x.di;
  words[6]        = r->x.cflag;
  if (s != nullptr) {
    words[7]  = s->es;
    words[8]  = s->cs;
    words[9]  = s->ss;
    words[10] = s->ds;
  }
  result     = service(ds, (s != nullptr) ? SoundServiceId::INT86X : SoundServiceId::INT86, words);
  r->x.ax    = words[0];
  r->x.bx    = words[1];
  r->x.cx    = words[2];
  r->x.dx    = words[3];
  r->x.si    = words[4];
  r->x.di    = words[5];
  r->x.cflag = words[6];
  if (s != nullptr) {
    s->es = words[7];
    s->cs = words[8];
    s->ss = words[9];
    s->ds = words[10];
  }
  return result;
}

auto host_segread(uint8_t* ds, SoundService service, SegRegs* s) -> void {
  uint16_t words[4] = { 0, 0, 0, 0 };
  service(ds, SoundServiceId::SEGREAD, words);
  s->es = words[0];
  s->cs = words[1];
  s->ss = words[2];
  s->ds = words[3];
}
}

auto opeu_effect_playing(uint8_t* ds, SoundService service) -> int;
void opeu_music_stop(uint8_t* ds, SoundService service);
void opeu_effect_stop(uint8_t* ds, SoundService service);
void opeu_effect_start(uint8_t* ds, SoundService service, uint8_t effect);
void opeu_music_load(uint8_t* ds, SoundService service, uint16_t bank, uint16_t path);
void opeu_music_select(uint8_t* ds, SoundService service, uint8_t value);

void opeu_sound_initialize(uint8_t* ds, SoundService service) {
  Data(ds).driver        = host_open(ds, service);
  Data(ds).sound_enabled = static_cast<uint16_t>(Data(ds).driver != 0);
}

void opeu_finish_side_initialization(uint8_t* ds, SoundService service, uint8_t value) {
  Regs r = {};
  if ((Data(ds).driver != 0u) && (Data(ds).sound_enabled != 0u)) {
    r.h.ah = 4;
    r.h.al = value;
    host_int86(ds, service, &r, nullptr);
  }
}

/* OPEN: no driver test. */
void opeu_music_stop(uint8_t* ds, SoundService service) {
  Regs r = {};
  r.h.ah = 2;
  r.h.al = 20;
  host_int86(ds, service, &r, nullptr);
}

/* OPEN: tests the driver like opeu_effect_playing. */
auto opeu_music_playing(uint8_t* ds, SoundService service) -> int {
  int  state = 0;
  Regs r     = {};
  if (Data(ds).driver == 0u) return 0;
  r.h.ah = 3;
  state  = host_int86(ds, service, &r, nullptr);
  return static_cast<int>((state & 0xff) != 0);
}

void opeu_music_start(uint8_t* ds, SoundService service, uint8_t track) {
  Regs r = {};
  if (Data(ds).driver == 0u) return;
  if (opeu_music_playing(ds, service) != 0) {
    if (Data(ds).music_current == track) return;
    opeu_music_stop(ds, service);
    while (opeu_music_playing(ds, service) != 0);
  }
  r.h.ah = 1;
  r.h.al = track + 1;
  host_int86(ds, service, &r, nullptr);
  Data(ds).music_current = track;
}

void opeu_set_turn_context(uint8_t* ds, SoundService service, uint8_t track) {
  if (Data(ds).sound_enabled != 0u) {
    opeu_music_start(ds, service, track);
    opeu_finish_side_initialization(ds, service, 1);
  }
}

void opeu_music_load(uint8_t* ds, SoundService service, uint16_t bank, uint16_t path) {
  Regs    r = {};
  SegRegs segs{};
  if (Data(ds).driver == 0u) return;
  host_copy(ds, bank, Near(ds, Data(ds).music_bank), 7);
  host_copy(ds, path, Near(ds, Data(ds).music_path), 13);
  host_segread(ds, service, &segs);
  r.h.al = 0;
  r.x.dx = Near(ds, Data(ds).music_bank);
  r.h.ah = 5;
  host_int86(ds, service, &r, &segs);
}

void opeu_music_select(uint8_t* ds, SoundService service, uint8_t value) {
  Regs r = {};
  if (Data(ds).driver != 0u) {
    r.h.al = 0;
    r.h.dl = value;
    r.h.ah = 6;
    host_int86(ds, service, &r, nullptr);
  }
}

void opeu_music_load_select(uint8_t* ds, SoundService service, uint16_t bank, uint16_t path, uint8_t value) {
  if (Data(ds).sound_enabled == 0u) return;
  opeu_music_load(ds, service, bank, path);
  opeu_music_select(ds, service, value);
}

/* OPEN: tests the driver. */
void opeu_music_command7(uint8_t* ds, SoundService service) {
  Regs r = {};
  if (Data(ds).driver != 0u) {
    r.h.al = 0;
    r.h.ah = 7;
    host_int86(ds, service, &r, nullptr);
  }
}

void opeu_effect_stop(uint8_t* ds, SoundService service) {
  Regs r = {};
  if (Data(ds).driver != 0u) {
    r.h.ah = 18;
    host_int86(ds, service, &r, nullptr);
  }
}

auto opeu_effect_playing(uint8_t* ds, SoundService service) -> int {
  int  state = 0;
  Regs r     = {};
  if (Data(ds).driver == 0u) return 0;
  r.h.ah = 19;
  state  = host_int86(ds, service, &r, nullptr);
  return static_cast<int>((state & 0xff) != 0);
}

void opeu_effect_load(uint8_t* ds, SoundService service, uint32_t data) {
  Regs    r = {};
  SegRegs segs{};
  if (Data(ds).driver != 0u) {
    r.h.ah  = 16;
    segs.ds = static_cast<uint16_t>(data >> 16);
    r.x.dx  = static_cast<uint16_t>(data);
    host_int86(ds, service, &r, &segs);
  }
}

void opeu_effect_start(uint8_t* ds, SoundService service, uint8_t effect) {
  Regs r = {};
  if (Data(ds).driver == 0u) return;
  if (opeu_effect_playing(ds, service) != 0) {
    if (Data(ds).effect_current == effect) return;
    opeu_effect_stop(ds, service);
    while (opeu_effect_playing(ds, service) != 0);
  }
  /* OPEN: always starts effect slot 1 and records the requested effect. */
  r.h.ah = 17;
  r.h.al = 1;
  host_int86(ds, service, &r, nullptr);
  Data(ds).effect_current = effect;
}

void opeu_effect_play(uint8_t* ds, SoundService service, uint8_t effect) {
  if (Data(ds).sound_enabled != 0u) opeu_effect_start(ds, service, effect);
}

}
