/* The KOEI FM driver (FMDRV.COM, "OPL-II Sound Driver Version 3.30"),
 * rewritten by hand as native C.
 *
 * The original is a TSR (src/fmdrv/driver.asm, recovered byte-exact)
 * that MAIN and OPEN call through INT 66h and a far-call entry. It sequences
 * song and sound-effect banks that live in the caller's memory and writes
 * YM3812 (OPL2) registers from the PIT interrupt. This module keeps its data
 * model and its behaviour -- the register stream it produces for a given
 * command sequence and tick sequence is the original's (port/tests/
 * fmdriver_diff.py compares the two) -- but it is plain C: named voice
 * records, functions per track opcode, and three host hooks instead of
 * ports and segment registers:
 *
 *   write_register(reg, value)  one OPL2 register write (port 388h/389h pair)
 *   read_status()               OPL2 status port, only for chip detection
 *   peek(seg, off)              one byte of the caller's memory (banks and
 *                               instruments are addressed segment:offset)
 *   callback(regs)              the client's end-of-song far callback
 *
 * Time: fmd_timer_tick() is one PIT interrupt of the original (the driver
 * programs PIT channel 0 to 1000h counts: 1193182/4096 = 291.304 Hz). The
 * host calls it at that rate (port/services/fm.c does so sample-accurately
 * from the audio clock). Calls into the driver must not overlap a tick; the
 * host serialises them (the original masked interrupts / used a busy flag).
 */
#pragma once

#include <stdint.h>

#define FMD_PIT_HZ_NUM 1193182u       /* PIT input clock */
#define FMD_PIT_DIVISOR 4096u          /* counts per driver tick */

/* Registers of one driver call (in and out). AH = function, AL = argument;
 * DS:DX is a far pointer argument (song bank, instrument, callback). */
typedef struct FmdRegs {
  uint16_t ax, bx, dx, ds;
} FmdRegs;

/* Registers handed to and returned by the client end-of-song callback:
 * AL = queued song, AH = current song, BL = queued repeat mode, BH = repeat
 * mode, CL = queued repeat mode, CH = 0, DX = callback offset | segment.
 * The driver then starts song AL (if nonzero) with repeat mode BL and
 * stores CH/CL as the new queued song / queued repeat mode. */
typedef struct FmdCallbackRegs {
  uint16_t ax, bx, cx, dx;
} FmdCallbackRegs;

typedef struct FmdHost {
  void* user;
  void (*write_register)(void* user, uint8_t reg, uint8_t value);
  uint8_t (*read_status)(void* user);
  uint8_t (*peek)(void* user, uint16_t seg, uint16_t off);
  void (*callback)(void* user, uint16_t seg, uint16_t off, FmdCallbackRegs* regs);
} FmdHost;

/* One voice record. Six melodic voices (0-3 music, 4-5 sound effects) on
 * OPL channels 0-5, then the rhythm voice (channel 6; channels 7 and 8 are
 * drums in rhythm mode). fmd_clear_voice() resets the fields above
 * opl_channel; the fields from opl_channel on survive it. */
typedef struct FmdVoice {
  uint16_t track;                 /* current track offset in the bank segment, 0 = ended */
  uint16_t loop;                  /* loop point (track start until FEh) */
  uint8_t flags;                  /* FMD_VOICE_* */
  uint8_t volume;                 /* track volume 0-7Fh */
  uint8_t note;                   /* last note (FFh none); rhythm voice: drums keyed on */
  uint8_t length;                 /* default note length (ticks) */
  uint8_t gate;                   /* default key-on time (ticks) */
  uint8_t length_count;
  uint8_t gate_count;
  uint8_t repeat_count;
  uint16_t repeat_start;
  uint16_t repeat_exit;
  uint16_t frequency;             /* unmodulated block/F-number (B0h:A0h) */
  uint8_t level[2];               /* operator levels after volume */
  uint8_t level_mod[2];           /* tremolo attenuation */
  uint16_t instrument_off, instrument_seg;
  uint8_t instrument_transpose;   /* signed semitones */
  uint8_t lfo_flags;              /* FMD_LFO_* */
  uint16_t lfo_rate;
  uint16_t lfo_depth;
  uint16_t lfo_count;
  uint16_t lfo_direction;         /* +1 / -1 */
  uint16_t lfo_phase;
  /* not cleared by fmd_clear_voice: */
  uint8_t opl_channel;            /* 0-8 */
  uint8_t opl_operator;           /* modulator operator offset (carrier = +3) */
  uint8_t ksl_tl[2];              /* cached registers 40h/43h + operator */
  uint8_t fnum;                   /* cached register A0h + channel */
  uint8_t keyon;                  /* cached register B0h + channel */
  uint8_t volume_offset;          /* API 80h; bit 7 mutes key-on */
  uint8_t transpose;              /* API 81h, signed semitones */
} FmdVoice;

/* FmdVoice.flags */
#define FMD_VOICE_WAITING 0x01    /* first event of the track not read yet */
#define FMD_VOICE_VIBRATO 0x02    /* software vibrato (F6h/F7h) set */
#define FMD_VOICE_LFO_AVAILABLE 0x04    /* instrument has an LFO */
#define FMD_VOICE_LFO_WANTED 0x08    /* F8h: run the instrument LFO */
#define FMD_VOICE_LEGATO 0x20    /* gate longer than length: tie the next note */
#define FMD_VOICE_SOUNDED 0x40    /* keyed on since the last silence */
#define FMD_VOICE_KEY_ON 0x80    /* key is on */

/* FmdVoice.lfo_flags */
#define FMD_LFO_STATE 0x03    /* 0 idle, 1 delay, 2 running, 3 (re)start */
#define FMD_LFO_DOWN 0x04    /* software vibrato bends downwards */
#define FMD_LFO_RAMP 0x40    /* software vibrato ramp in progress */
#define FMD_LFO_ON 0x80    /* instrument LFO enabled */

#define FMD_VOICES 6
#define FMD_RHYTHM 6

typedef struct FmDriver {
  FmdHost host;
  FmdVoice voice[FMD_VOICES + 1];     /* [FMD_RHYTHM] is the rhythm voice */

    /* timer */
  uint16_t tempo_increment;           /* added to tempo_accumulator per PIT tick */
  uint16_t tempo_accumulator;
  uint16_t pending_music_ticks;       /* accumulator carries not yet sequenced */
  uint16_t pending_fixed_ticks;
  uint8_t fixed_tick_accumulator;     /* += 36h per PIT tick */
  uint8_t bios_tick_divider;          /* every 16th PIT tick goes to the BIOS */
  uint8_t countdown_accumulator[2];   /* += 35h per PIT tick */
  uint16_t countdown[2];              /* INT 66h F0h/F1h */
  uint8_t enabled;                    /* FFh once an OPL chip was found */
  uint8_t busy;                       /* nonzero: the tick must not sequence */

    /* client data */
  uint16_t music_off, music_seg;      /* song bank (API 00h) */
  uint16_t effects_off, effects_seg;  /* sound-effect bank (API 10h) */
  uint16_t callback_off, callback_seg;/* end-of-song callback (API 09h) */

    /* song state */
  uint16_t fixed_tick_count;          /* fixed ticks since install (LFO seed) */
  uint16_t random_state;              /* sample-and-hold LFO generator */
  uint16_t music_ticks;               /* API 03h */
  uint16_t effect_ticks;              /* API 13h */
  uint8_t tempo;                      /* beats per minute (0 = stopped) */
  uint8_t fade_length;                /* API 02h; 0 = no fade */
  uint8_t fade_prescaler;             /* the fade advances every 4 fixed ticks */
  uint8_t fade_step;
  uint8_t fade_attenuation;           /* 0-FFh */
  uint8_t repeat_mode;                /* API 04h */
  uint8_t current_song;               /* 0 = none */
  uint8_t queued_repeat;              /* API 08h */
  uint8_t queued_song;
  uint8_t current_effect;

  /* rhythm mode: drum levels from the instrument set, per-drum velocity */
  uint8_t level_bd, level_hh, level_sd, level_tom, level_cym;
  uint8_t velocity_bd, velocity_hh, velocity_sd, velocity_tom, velocity_cym;
  uint8_t rhythm_control;             /* shadow of register BDh (AM/VIB depth) */
} FmDriver;

/* Install: reset the state; with detect != 0 run the original timer-overflow
 * chip test through read_status (an emulated chip may skip it). Returns 1
 * when the driver is enabled (chip found), 0 otherwise; then only the
 * countdowns and the version query work and other calls return AX=FFFFh. */
int fmd_install(FmDriver* d, const FmdHost* host, int detect);
/* INT 66h AH=FFh / the far uninstall entry: silence the chip. */
void fmd_uninstall(FmDriver* d);
/* INT 66h (full interface incl. FFh/FEh/F0h/F1h). */
void fmd_interrupt(FmDriver* d, FmdRegs* r);
/* The far-call entry (CS:010Ah): the API of functions 00-09h, 10-13h, 80-83h. */
void fmd_far_call(FmDriver* d, FmdRegs* r);
/* One PIT interrupt (291.3 Hz). */
void fmd_timer_tick(FmDriver* d);
