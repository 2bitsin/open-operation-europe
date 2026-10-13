/* INT 66h: the KOEI FM driver (FMDRV, src/fmdrv/driver.asm) for the port.
 *
 * The driver's logic is the native rewrite in fmdriver.c; its register
 * writes go to the OPL2 chip (opl.hpp, ymfm) and the chip's samples to the
 * SDL3 audio stream. This file is the hardware side the original driver
 * found on the PC: the PIT that clocked it, the OPL ports, the memory it
 * read the banks from, and the audio output.
 *
 * Clock. The original programmed PIT channel 0 to 1000h counts, 291.304 Hz.
 * Here the driver tick is derived from the audio sample clock (49716 Hz,
 * the OPL2's own rate), sample-accurately: every output frame adds 1193182
 * to a phase accumulator and a PIT tick falls due each 4096 * 49716 units,
 * before the frame at which it is due. The same function clocks the driver
 * in every mode:
 *   - window with an audio device: the SDL audio callback (audio thread,
 *     or the browser's audio callback on the web) renders frames, so the
 *     music and the countdowns run while the game computes, as the PIT
 *     interrupt did. Game-side driver calls take the audio lock. If the
 *     device stops calling back for 200 ms the event pump clocks the
 *     driver from host time instead (no chip samples), so the countdown
 *     waits never hang.
 *   - headless (and window without an audio device): the event pump clocks
 *     the driver up to port_time_us() (virtual time headless: exactly 1 ms
 *     per poll, so runs are reproducible); chip samples are produced only
 *     when they are written to --wav.
 *
 * Entry points: port_fm_function (the INT 66h subset of the SYSLIB helpers:
 * countdowns, bank load) and port_fm_int66 (MAIN's SOUND module, full
 * registers). MAIN never uses the far-call entry or a client callback.
 */
#include "fmdriver.hpp"
#include "hostinput.hpp"
#include "opl.hpp"
#include "platform.hpp"
#include "port.hpp"
#include "services.hpp"

#include <span>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
constexpr auto PIT_UNITS_PER_FRAME{ FMD_PIT_HZ_NUM };
constexpr auto PIT_UNITS_PER_TICK{ FMD_PIT_DIVISOR * (uint32_t)OPL_SAMPLE_RATE };
constexpr auto STALL_US{ 200000u };
}

static FmDriver driver;
static int      installed;
static int      audio_device;  /* 1: the audio callback clocks the driver */
static uint32_t tick_phase;    /* PIT input clock units since the last tick */
static uint64_t clock_frames;  /* output frames the driver clock has advanced */
static uint64_t last_clock_us; /* host time of the last callback / pump clocking */
static FILE*    wav;
static uint64_t wav_frames;
static FILE*    opl_log;

/* ---- driver host hooks ---------------------------------------------------- */

static void HostWriteRegister(void* user, uint8_t reg, uint8_t value) {
  (void)user;
  OplWrite(reg, value);
  if (opl_log) fprintf(opl_log, "%llu %02x %02x\n", (unsigned long long)clock_frames, reg, value);
}

static uint8_t HostReadStatus(void* user) {
  (void)user;
  return 0;
}

/* The banks are in the data arena (MAIN's FMUSIC at 3D91:0000/7BBA). */
static uint8_t HostPeek(void* user, uint16_t seg, uint16_t off) {
  uint32_t linear = (uint32_t)seg * 16u + off;
  (void)user;
  return linear < PORT_ARENA_SIZE ? port_arena[linear] : 0xff;
}

static void HostCallback(void* user, uint16_t seg, uint16_t off, FmdCallbackRegs* regs) {
  (void)user;
  (void)regs;
  port_trace("fm", "client callback %04x:%04x ignored (MAIN installs none)", seg, off);
}

/* ---- the clock -------------------------------------------------------------- */

static void WavHeader(FILE* f, uint64_t frames) {
  uint32_t data = (uint32_t)(frames * 4u), rate = OPL_SAMPLE_RATE;
  uint8_t  h[44];
  memcpy(h, "RIFF", 4);
  h[4] = (uint8_t)(data + 36);
  h[5] = (uint8_t)((data + 36) >> 8);
  h[6] = (uint8_t)((data + 36) >> 16);
  h[7] = (uint8_t)((data + 36) >> 24);
  memcpy(h + 8, "WAVEfmt ", 8);
  h[16] = 16;
  h[17] = h[18] = h[19] = 0;
  h[20]                 = 1;
  h[21]                 = 0; /* PCM */
  h[22]                 = 2;
  h[23]                 = 0; /* stereo */
  h[24]                 = (uint8_t)rate;
  h[25]                 = (uint8_t)(rate >> 8);
  h[26]                 = (uint8_t)(rate >> 16);
  h[27]                 = 0;
  h[28]                 = (uint8_t)(rate * 4);
  h[29]                 = (uint8_t)((rate * 4) >> 8);
  h[30]                 = (uint8_t)((rate * 4) >> 16);
  h[31]                 = (uint8_t)((rate * 4) >> 24);
  h[32]                 = 4;
  h[33]                 = 0; /* block align */
  h[34]                 = 16;
  h[35]                 = 0; /* bits */
  memcpy(h + 36, "data", 4);
  h[40] = (uint8_t)data;
  h[41] = (uint8_t)(data >> 8);
  h[42] = (uint8_t)(data >> 16);
  h[43] = (uint8_t)(data >> 24);
  fwrite(h, 1, sizeof h, f);
}

/* Advance the driver clock by `frames` output frames. PIT ticks run at their
 * exact frame positions; chip samples go to `out` (NULL: not rendered, the
 * chip only receives the register writes) and to the WAV file. */
static void ClockFramesAdvance(int16_t* out, int frames) {
  static int16_t scratch[2 * 512];
  while (frames > 0) {
    uint32_t need  = PIT_UNITS_PER_TICK - tick_phase;
    uint32_t until = (need + PIT_UNITS_PER_FRAME - 1) / PIT_UNITS_PER_FRAME;
    int      n     = (int)(until < (uint32_t)frames ? until : (uint32_t)frames);
    if (tick_phase >= PIT_UNITS_PER_TICK) n = 0;
    if (n > 512) n = 512;
    if (n > 0) {
      int16_t* dst = out ? out : (wav ? scratch : NULL);
      if (dst) OplRender(dst, n);
      if (wav) {
        fwrite(dst, 4, (size_t)n, wav);
        wav_frames += (uint64_t)n;
      }
      if (out) out += 2 * n;
      tick_phase   += (uint32_t)n * PIT_UNITS_PER_FRAME;
      clock_frames += (uint64_t)n;
      frames       -= n;
    }
    if (tick_phase >= PIT_UNITS_PER_TICK) {
      tick_phase -= PIT_UNITS_PER_TICK;
      if (installed) FmdTimerTick(&driver);
    }
  }
}

/* SDL audio callback (the stream lock is held). */
static void AudioCallback(std::span<int16_t> stereo) {
  ClockFramesAdvance(stereo.data(), static_cast<int>(stereo.size() / 2));
  last_clock_us = opeu::platform::TicksUs();
}

/* Called by every event pump. */
void port_fm_clock(void) {
  uint64_t now;
  if (!installed) return;
  if (audio_device) {
    opeu::platform::AudioLock const lock;
    now = opeu::platform::TicksUs(); /* after the lock: the callback may just have run */
    if (now > last_clock_us && now - last_clock_us > STALL_US) {
      /* the device stopped pulling samples: keep the driver's time */
      uint64_t frames = (now - last_clock_us) * OPL_SAMPLE_RATE / 1000000u;
      ClockFramesAdvance(NULL, (int)(frames > OPL_SAMPLE_RATE ? OPL_SAMPLE_RATE : frames)); /* at most 1 s per poll */
      last_clock_us = now;
    }
    return;
  }
  now = port_time_us();
  {
    uint64_t target = now * OPL_SAMPLE_RATE / 1000000u;
    while (clock_frames < target) {
      uint64_t n = target - clock_frames;
      ClockFramesAdvance(NULL, (int)(n > 4096 ? 4096 : n));
    }
  }
}

static void FmShutdown(void) {
  opeu::platform::AudioLock const lock;
  if (wav) {
    fseek(wav, 0, SEEK_SET);
    WavHeader(wav, wav_frames);
    fclose(wav);
    wav = NULL;
  }
  if (opl_log) {
    fclose(opl_log);
    opl_log = NULL;
  }
}

/* Install the driver (FMDRV ran from OPEU.BAT before MAIN) and the audio. */
void port_fm_init(void) {
  FmdHost host;
  host.user           = NULL;
  host.write_register = HostWriteRegister;
  host.read_status    = HostReadStatus;
  host.peek           = HostPeek;
  host.callback       = HostCallback;
  if (port_options.wav_path) {
    wav = fopen(port_options.wav_path, "wb");
    if (!wav) port_fatal("cannot write %s", port_options.wav_path);
    WavHeader(wav, 0);
  }
  if (port_options.opl_log_path) {
    opl_log = fopen(port_options.opl_log_path, "w");
    if (!opl_log) port_fatal("cannot write %s", port_options.opl_log_path);
    fprintf(opl_log, "# frame (at %d Hz) register value\n", OPL_SAMPLE_RATE);
  }
  OplReset();
  /* The emulated chip is always there; the timer-overflow detection of
     * the original installer is not run. */
  FmdInstall(&driver, &host, 0);
  installed = 1;
  atexit(FmShutdown);
  if (!port_options.headless && !port_options.mute) {
    audio_device  = opeu::platform::OpenAudio(OPL_SAMPLE_RATE, AudioCallback) ? 1 : 0;
    last_clock_us = opeu::platform::TicksUs();
  }
  port_trace("fm", "driver installed; clock: %s", audio_device ? "audio device" : "event pump");
  if (!port_options.headless && !port_options.mute && !audio_device) fprintf(stderr, "opeu: no audio output; the music is silent\n");
}

/* ---- INT 66h ------------------------------------------------------------------ */

static void DriverCall(FmdRegs* r) {
  if (!installed) {
    r->ax = 0xffff; /* no driver resident */
    return;
  }
  opeu::platform::AudioLock const lock;
  FmdInterrupt(&driver, r);
}

/* Full-register INT 66h (port_sound_int66, port/core/glue.c). */
void port_fm_int66(uint16_t* ax, uint16_t* bx, uint16_t* dx, uint16_t* ds) {
  FmdRegs r;
  uint8_t ah = (uint8_t)(*ax >> 8);
  r.ax       = *ax;
  r.bx       = *bx;
  r.dx       = *dx;
  r.ds       = *ds;
  DriverCall(&r);
  port_trace("fm", "frame %llu int66 ax=%04x bx=%04x dx=%04x ds=%04x -> ax=%04x bx=%04x", (unsigned long long)clock_frames, *ax, *bx, *dx, *ds, r.ax, r.bx);
  *ax = r.ax;
  *bx = r.bx;
  *dx = r.dx;
  *ds = r.ds;
  /* the callers busy-wait on the status functions and on the countdowns
     * (a countdown read that returns 0 ends the wait: no poll) */
  if (ah == 0x03 || ah == 0x13 || ((ah == 0xf0 || ah == 0xf1) && (uint8_t)r.ax == 0 && r.bx != 0)) port_pump();
}

/* INT 66h for the SYSLIB helpers: AH, AL, BX, DS:DX in; the countdown reads
 * (F0h/F1h AL=0) return BX, everything else AX. */
uint16_t port_fm_function(uint8_t ah, uint8_t al, uint16_t bx, uint16_t dx_off, uint16_t ds_seg) {
  uint16_t ax = (uint16_t)(ah << 8 | al), dx = dx_off, ds = ds_seg;
  port_fm_int66(&ax, &bx, &dx, &ds);
  if ((ah == 0xf0 || ah == 0xf1) && al == 0) return bx;
  return (ah == 0xf0 || ah == 0xf1) ? 0 : ax;
}
