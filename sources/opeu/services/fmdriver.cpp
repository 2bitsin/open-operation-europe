/* The KOEI FM driver (FMDRV.COM 3.30) as native C. See fmdriver.h.
 *
 * Hand port of src/fmdrv/driver.asm: the same voice records, sequencer,
 * fade, queue, callback, LFO and rhythm logic, expressed as C functions on
 * named fields. Arithmetic keeps the original's widths (8/16-bit wrap,
 * unsigned vs signed multiply/divide) because the register stream depends
 * on them; each function names the original routine it corresponds to
 * (driver.asm offset in brackets). Nothing of the original binary is used
 * or executed.
 *
 * Bank formats (from the driver's reads):
 *   song bank:   song n at (n-1)*10: five words, the track offsets of music
 *                voices 0-3 and of the rhythm voice relative to the entry
 *                (0 = no track);
 *   effect bank: effect n at (n-1)*4: two words, the tracks of effect
 *                voices 4-5 relative to the entry;
 *   instrument:  22 bytes: [0..10] operator registers 20/23, C0, E0/E3,
 *                40/43, 60/63, 80/83; [11] transpose; [12] word LFO rate;
 *                [14] LFO shape (0 saw, 1 square, 2 triangle, 3 random,
 *                4 ramp); [15] pitch depth (signed); [16] depth step;
 *                [17] level depth (signed); [18]/[19] level scale per
 *                operator; [20] LFO delay; [21] LFO enable;
 *   rhythm set:  three groups (BD on channel 6, HH/SD on 7, TOM/CYM on 8)
 *                of A0h value, B0h value and 11 operator bytes.
 */
#include "fmdriver.hpp"

#include <stddef.h>
#include <string.h>

/* ---- constants ----------------------------------------------------------- */

/* Melody track commands (bytes C0h-FFh; 00-5Fh are notes with explicit
 * length and gate, 60h-BFh notes with the default length and gate). */
enum {
    MEL_AM_DEPTH = 0xee, MEL_VIBRATO_DEPTH, MEL_VOLUME, MEL_TEMPO_PERIOD, MEL_TEMPO,
    MEL_SKIP1, MEL_INSTRUMENT, MEL_SKIP2, MEL_VIBRATO_TEMPO, MEL_VIBRATO_RATE,
    MEL_LFO_ON, MEL_LFO_OFF, MEL_NOP_FA, MEL_NOP_FB, MEL_NOP_FC,
    MEL_REPEAT_BREAK, MEL_LOOP_POINT, MEL_END
};

/* Rhythm track commands (bytes 80h-FFh; 00-7Fh are drum events). */
enum {
    RHY_VOLUME = 0xf0, RHY_SKIP1_F1, RHY_TEMPO, RHY_SKIP1_F3, RHY_INSTRUMENTS,
    RHY_SKIP2_F5, RHY_SKIP2_F6, RHY_SKIP2_F7,
    RHY_REPEAT_BREAK = 0xfd, RHY_LOOP_POINT, RHY_END
};

/* Drum bits of register BDh / of the rhythm events. */
#define DRUM_HH  0x01
#define DRUM_CYM 0x02
#define DRUM_TOM 0x04
#define DRUM_SD  0x08
#define DRUM_BD  0x10

/* [0710] vibrato depths (index 0-12) and [072a]... LFO depth steps. */
static const uint16_t modulation_depth_table[13] = {
    0x0000, 0x079c, 0x0fad, 0x1838, 0x2145, 0x2adc, 0x3505, 0x3fc8, 0x4b2f, 0x5744, 0x6411, 0x71a1, 0x7fff
};
static const uint16_t modulation_step_table[16] = {
    0x004c, 0x0072, 0x00be, 0x010a, 0x017d, 0x02fe, 0x079c, 0x0fad,
    0x2145, 0x2adc, 0x3fc8, 0x4b2f, 0x5744, 0x7fff, 0x9f58, 0xea08
};
/* OPL F-numbers of C, C#, ... B. */
static const uint16_t note_frequency_table[12] = {
    0x0157, 0x016b, 0x0181, 0x0198, 0x01b0, 0x01ca, 0x01e5, 0x0202, 0x0220, 0x0241, 0x0263, 0x0287
};

/* OPL channel and modulator operator offset of each voice record. */
static const uint8_t voice_channel[FMD_VOICES + 1] = {0, 1, 2, 3, 4, 5, 6};
static const uint8_t voice_operator[FMD_VOICES + 1] = {0x00, 0x01, 0x02, 0x08, 0x09, 0x0a, 0x00};

/* ---- host access --------------------------------------------------------- */

static void opl(FmDriver *d, uint8_t reg, uint8_t value)
{
    d->host.write_register(d->host.user, reg, value);
}

static uint8_t peek(FmDriver *d, uint16_t seg, uint16_t off)
{
    return d->host.peek(d->host.user, seg, off);
}

static uint16_t peek_word(FmDriver *d, uint16_t seg, uint16_t off)
{
    return (uint16_t)(peek(d, seg, off) | peek(d, seg, (uint16_t)(off + 1)) << 8);
}

static FmdVoice *effect_voice(FmDriver *d, unsigned n)
{
    return &d->voice[4 + n];
}

/* ---- OPL register helpers ------------------------------------------------ */

/* [0768] voice_key_on: set KEY-ON unless the voice is muted (offset bit 7). */
static void voice_key_on(FmDriver *d, FmdVoice *v)
{
    if (v->volume_offset & 0x80)
        return;
    v->keyon |= 0x20;
    opl(d, (uint8_t)(0xb0 + v->opl_channel), v->keyon);
    v->flags |= FMD_VOICE_KEY_ON | FMD_VOICE_SOUNDED;
    v->lfo_flags |= FMD_LFO_STATE;              /* restart the LFO */
}

/* [078a] voice_key_off. */
static void voice_key_off(FmDriver *d, FmdVoice *v)
{
    v->keyon &= 0xdf;
    opl(d, (uint8_t)(0xb0 + v->opl_channel), v->keyon);
    v->flags &= (uint8_t)~(FMD_VOICE_KEY_ON | FMD_VOICE_LEGATO);
    v->lfo_flags &= (uint8_t)~FMD_LFO_RAMP;
}

/* [07a6] voice_silence: frequency 0, fastest release, full attenuation. */
static void voice_silence(FmDriver *d, FmdVoice *v)
{
    uint8_t op = v->opl_operator;
    v->fnum = 0;
    v->keyon = 0;
    opl(d, (uint8_t)(0xa0 + v->opl_channel), 0);
    opl(d, (uint8_t)(0xb0 + v->opl_channel), 0);
    opl(d, (uint8_t)(0x80 + op), 0x0f);
    opl(d, (uint8_t)(0x83 + op), 0x0f);
    opl(d, (uint8_t)(0x40 + op), 0x3f);
    v->ksl_tl[0] = 0x3f;
    opl(d, (uint8_t)(0x43 + op), 0x3f);
    v->ksl_tl[1] = 0x3f;
    v->flags &= 0x1f;                           /* key, sounded, (40h) cleared */
    v->lfo_flags &= (uint8_t)~FMD_LFO_RAMP;
}

/* [0836] load_operator_pair: 11 instrument bytes to the registers of one
 * channel (20/23, C0, E0/E3, 40/43, 60/63, 80/83). */
static void load_operator_pair(FmDriver *d, uint16_t seg, uint16_t off, uint8_t op, uint8_t channel)
{
    static const uint8_t base[11] = {0x20, 0x23, 0xc0, 0xe0, 0xe3, 0x40, 0x43, 0x60, 0x63, 0x80, 0x83};
    int i;
    for (i = 0; i < 11; i++) {
        uint8_t reg = (uint8_t)(base[i] + (i == 2 ? channel : op));
        opl(d, reg, peek(d, seg, (uint16_t)(off + i)));
    }
}

/* [0892] write_operator_level: level (0 loud .. 3Fh silent) minus the
 * tremolo attenuation, written to 40h/43h+op if it changed. */
static void write_operator_level(FmDriver *d, FmdVoice *v, int which, uint8_t reg, uint8_t level)
{
    uint8_t loudness = (uint8_t)(~level & 0x3f);
    uint8_t cut = (uint8_t)((unsigned)loudness * v->level_mod[which] / 0x7f);
    uint8_t value, old;
    loudness = loudness >= cut ? (uint8_t)(loudness - cut) : 0;
    old = v->ksl_tl[which];
    value = (uint8_t)((~loudness & 0x3f) | (old & 0xc0));
    if (value != old) {
        v->ksl_tl[which] = value;
        opl(d, reg, value);
    }
}

/* [08c2] write_frequency: A0h/B0h+channel when the block/F-number changed;
 * the KEY-ON bit is kept. */
static void write_frequency(FmDriver *d, FmdVoice *v, uint16_t frequency)
{
    uint16_t want = frequency & 0xdfff;
    uint16_t have = (uint16_t)((v->fnum | v->keyon << 8) & 0xdfff);
    if (want == have)
        return;
    v->fnum = (uint8_t)want;
    opl(d, (uint8_t)(0xa0 + v->opl_channel), v->fnum);
    v->keyon = (uint8_t)((v->keyon & 0x20) | want >> 8);
    opl(d, (uint8_t)(0xb0 + v->opl_channel), v->keyon);
}

/* ---- tempo, fade, voice state -------------------------------------------- */

/* [08f3] set_tempo: bpm (0 = keep); the timer adds bpm*464Eh/100 per PIT
 * tick to the 16-bit tempo accumulator, one music tick per carry. */
static void set_tempo(FmDriver *d, uint8_t bpm)
{
    if (bpm == 0)
        return;
    d->tempo = bpm;
    d->tempo_increment = (uint16_t)((uint32_t)bpm * 0x464e / 100);
    d->tempo_accumulator = 0;
}

/* [0913] apply_fade: volume plus the fade attenuation, clamped to 7Fh. */
static uint8_t apply_fade(const FmDriver *d, uint8_t volume)
{
    uint8_t a = d->fade_attenuation;
    unsigned sum;
    a = a >= 0x80 ? (uint8_t)(a - 0x40) : (uint8_t)(a >> 1);
    sum = (unsigned)volume + a;
    return sum > 0x7f ? 0x7f : (uint8_t)sum;
}

/* [099e] clear_channel. */
static void clear_voice(FmdVoice *v)
{
    size_t keep = offsetof(FmdVoice, opl_channel);
    memset(v, 0, keep);
    v->flags = FMD_VOICE_WAITING;
    v->note = 0xff;
    v->length = 0x30;
    v->gate = 0x2d;
    v->length_count = 1;
    v->lfo_direction = 1;
}

/* [0a65] voice_update_volume: operator levels from the instrument and the
 * volume (plus offset; music voices 0-3 also the fade). The carrier always
 * follows the volume, the modulator only in additive mode (C0h bit 0). */
static void voice_update_volume(FmDriver *d, FmdVoice *v, uint8_t volume)
{
    unsigned sum = (unsigned)volume + (v->volume_offset & 0x7f);
    uint8_t attenuation, scaled;
    int i;
    volume = sum > 0x7f ? 0x7f : (uint8_t)sum;
    if (v->opl_channel < 4)
        volume = apply_fade(d, volume);
    attenuation = (uint8_t)(volume >> 1);
    if (!(v->instrument_seg | v->instrument_off))
        return;
    scaled = (uint8_t)(peek(d, v->instrument_seg, (uint16_t)(v->instrument_off + 2)) | 2);
    for (i = 0; i < 2; i++) {
        uint8_t level = (uint8_t)(peek(d, v->instrument_seg, (uint16_t)(v->instrument_off + 5 + i)) & 0x3f);
        if (scaled & 1) {
            level = (uint8_t)(level + attenuation);
            if (level > 0x3f)
                level = 0x3f;
        }
        scaled >>= 1;
        v->level[i] = level;
        write_operator_level(d, v, i, (uint8_t)(0x40 + v->opl_operator + 3 * i), level);
    }
}

/* [0ac2] voice_reset_level_modulation. */
static void voice_reset_level_modulation(FmDriver *d, FmdVoice *v)
{
    v->level_mod[0] = 0;
    v->level_mod[1] = 0;
    voice_update_volume(d, v, v->volume);
}

/* [0c01] voice_restore_frequency: unmodulated pitch and levels. */
static void voice_restore_frequency(FmDriver *d, FmdVoice *v)
{
    write_frequency(d, v, v->frequency);
    voice_reset_level_modulation(d, v);
}

/* [0b07] voice_set_instrument: program the operators from seg:off and set
 * up the instrument LFO. */
static void voice_set_instrument(FmDriver *d, FmdVoice *v, uint16_t seg, uint16_t off)
{
    v->instrument_off = off;
    v->instrument_seg = seg;
    voice_silence(d, v);
    load_operator_pair(d, seg, off, v->opl_operator, v->opl_channel);
    v->ksl_tl[0] = peek(d, seg, (uint16_t)(off + 5));
    v->ksl_tl[1] = peek(d, seg, (uint16_t)(off + 6));
    v->instrument_transpose = peek(d, seg, (uint16_t)(off + 11));
    v->lfo_flags &= (uint8_t)~FMD_LFO_ON;
    v->flags &= (uint8_t)~FMD_VOICE_LFO_AVAILABLE;
    if (peek(d, seg, (uint16_t)(off + 21)) != 0) {
        v->flags |= FMD_VOICE_LFO_AVAILABLE;
        if (v->flags & FMD_VOICE_LFO_WANTED) {
            v->lfo_flags |= FMD_LFO_ON;
            v->lfo_count = 0;
            v->lfo_phase = 0;
            v->lfo_direction = 1;
        }
    }
    voice_reset_level_modulation(d, v);
}

/* [0d20] voice_start_note: note 0-5Fh (gate 0 = rest): transposed, the
 * F-number looked up, keyed on (a legato voice keeps the key and does not
 * retrigger the same note); then the length and gate counters start. */
static void voice_start_note(FmDriver *d, FmdVoice *v, uint8_t note, uint8_t gate)
{
    if (gate == 0) {
        v->note = 0xff;
        if (v->flags & FMD_VOICE_KEY_ON)
            voice_key_off(d, v);
    } else {
        uint16_t total = (uint16_t)(note + (int8_t)v->instrument_transpose + (int8_t)v->transpose);
        int octave = (int16_t)total / 12;       /* idiv: truncating */
        int semitone = (int16_t)total % 12;
        uint8_t block, key;
        int same = 0;
        if ((total & 0xff00) == 0) {
            block = (uint8_t)octave;
            key = (uint8_t)total;
            if (block > 7) {                    /* above the top octave: clamp to block 7 */
                block = 7;
                key = (uint8_t)(semitone + 0x54);
            }
        } else {                                /* below C0 (or >= 256): block 0 */
            block = 0;
            if (semitone != 0)
                semitone += 12;
            key = (uint8_t)semitone;
        }
        if (v->flags & FMD_VOICE_LEGATO) {
            if (key == v->note)
                same = 1;
        } else if (v->flags & FMD_VOICE_KEY_ON) {
            voice_key_off(d, v);
        }
        if (!same) {
            /* The original indexes its 12-entry table with the semitone; a
             * transposed note >= 256 would index past it (into code bytes).
             * That needs transposes summing to over 160 semitones; here the
             * index wraps instead. */
            uint16_t frequency = (uint16_t)(note_frequency_table[(unsigned)semitone % 12] | block << 10);
            v->note = key;
            write_frequency(d, v, frequency);
            v->frequency = frequency;
            if (!(v->flags & FMD_VOICE_LEGATO))
                voice_key_on(d, v);
            v->lfo_flags &= (uint8_t)~FMD_LFO_RAMP;
        }
        if (v->flags & FMD_VOICE_VIBRATO)
            v->lfo_flags |= FMD_LFO_RAMP;
    }
    v->length_count = v->length;
    v->gate_count = v->gate;
    v->flags &= (uint8_t)~(FMD_VOICE_LEGATO | FMD_VOICE_VIBRATO);
    if (v->gate > v->length)
        v->flags |= FMD_VOICE_LEGATO;
}

/* ---- rhythm mode ----------------------------------------------------------- */

/* [07e8] rhythm_reset: BDh rhythm mode with no drum keyed; channels 6-8
 * silenced (frequency 0, fastest release, full attenuation). */
static void rhythm_reset(FmDriver *d)
{
    int i;
    opl(d, 0xbd, (uint8_t)((d->rhythm_control & 0xe0) | 0x20));
    d->voice[FMD_RHYTHM].note = 0x20;
    for (i = 0; i < 3; i++) {
        opl(d, (uint8_t)(0xa6 + i), 0);
        opl(d, (uint8_t)(0xb6 + i), 0);
    }
    for (i = 0; i < 3; i++) {
        opl(d, (uint8_t)(0x90 + i), 0x0f);
        opl(d, (uint8_t)(0x93 + i), 0x0f);
    }
    for (i = 0; i < 3; i++) {
        opl(d, (uint8_t)(0x50 + i), 0x3f);
        opl(d, (uint8_t)(0x53 + i), 0x3f);
    }
}

/* [0e4d] rhythm_load_instruments: program channels 6-8 from a rhythm set
 * and remember each drum's base level. */
static void rhythm_load_instruments(FmDriver *d, uint16_t seg, uint16_t off)
{
    rhythm_reset(d);
    /* bass drum: channel 6, operators 10h/13h */
    opl(d, 0xa6, peek(d, seg, off));
    opl(d, 0xb6, (uint8_t)(peek(d, seg, (uint16_t)(off + 1)) & 0x1f));
    off = (uint16_t)(off + 2);
    d->level_bd = (uint8_t)(peek(d, seg, (uint16_t)(off + 6)) & 0x3f);
    load_operator_pair(d, seg, off, 0x10, 6);
    off = (uint16_t)(off + 11);
    /* hi-hat (modulator) and snare (carrier): channel 7, operators 11h/14h */
    opl(d, 0xa7, peek(d, seg, off));
    opl(d, 0xb7, (uint8_t)(peek(d, seg, (uint16_t)(off + 1)) & 0x1f));
    off = (uint16_t)(off + 2);
    d->level_hh = (uint8_t)(peek(d, seg, (uint16_t)(off + 5)) & 0x3f);
    d->level_sd = (uint8_t)(peek(d, seg, (uint16_t)(off + 6)) & 0x3f);
    load_operator_pair(d, seg, off, 0x11, 7);
    off = (uint16_t)(off + 11);
    /* tom (modulator) and cymbal (carrier): channel 8, operators 12h/15h */
    opl(d, 0xa8, peek(d, seg, off));
    opl(d, 0xb8, (uint8_t)(peek(d, seg, (uint16_t)(off + 1)) & 0x1f));
    off = (uint16_t)(off + 2);
    d->level_tom = (uint8_t)(peek(d, seg, (uint16_t)(off + 5)) & 0x3f);
    d->level_cym = (uint8_t)(peek(d, seg, (uint16_t)(off + 6)) & 0x3f);
    load_operator_pair(d, seg, off, 0x12, 8);
}

static uint8_t drum_level(uint8_t base, uint8_t attenuation)
{
    unsigned sum = (unsigned)base + attenuation;
    return sum > 0x3f ? 0x3f : (uint8_t)sum;
}

/* Rhythm voice volume (plus offset and fade) as an attenuation 0-3Fh. */
static uint8_t rhythm_attenuation(FmDriver *d, const FmdVoice *v, uint8_t volume)
{
    unsigned sum = (unsigned)volume + (v->volume_offset & 0x7f);
    volume = sum > 0x7f ? 0x7f : (uint8_t)sum;
    return (uint8_t)(apply_fade(d, volume) >> 1);
}

/* [0ec5] rhythm_set_velocity: velocity for the drums in `drums`; their
 * levels are written at once. */
static void rhythm_set_velocity(FmDriver *d, FmdVoice *v, uint8_t drums, uint8_t velocity)
{
    uint8_t attenuation;
    velocity = (uint8_t)((velocity & 0x7f) >> 1);
    attenuation = (uint8_t)(rhythm_attenuation(d, v, v->volume) + velocity);
    if (drums & DRUM_HH) {
        d->velocity_hh = velocity;
        opl(d, 0x51, drum_level(d->level_hh, attenuation));
    }
    if (drums & DRUM_CYM) {
        d->velocity_cym = velocity;
        opl(d, 0x55, drum_level(d->level_cym, attenuation));
    }
    if (drums & DRUM_TOM) {
        d->velocity_tom = velocity;
        opl(d, 0x52, drum_level(d->level_tom, attenuation));
    }
    if (drums & DRUM_SD) {
        d->velocity_sd = velocity;
        opl(d, 0x54, drum_level(d->level_sd, attenuation));
    }
    if (drums & DRUM_BD) {
        d->velocity_bd = velocity;
        opl(d, 0x53, drum_level(d->level_bd, attenuation));
    }
}

/* [0f64] rhythm_update_volume: new rhythm volume; all five drum levels. */
static void rhythm_update_volume(FmDriver *d, FmdVoice *v, uint8_t volume)
{
    uint8_t attenuation;
    v->volume = (uint8_t)(volume & 0x7f);
    attenuation = rhythm_attenuation(d, v, v->volume);
    opl(d, 0x53, drum_level((uint8_t)(d->level_bd + d->velocity_bd), attenuation));
    opl(d, 0x54, drum_level((uint8_t)(d->level_sd + d->velocity_sd), attenuation));
    opl(d, 0x52, drum_level((uint8_t)(d->level_tom + d->velocity_tom), attenuation));
    opl(d, 0x55, drum_level((uint8_t)(d->level_cym + d->velocity_cym), attenuation));
    opl(d, 0x51, drum_level((uint8_t)(d->level_hh + d->velocity_hh), attenuation));
}

/* [1094] rhythm_key_on: retrigger the drums in `drums` through BDh: first
 * their key bits off, then on together with the drums already sounding. */
static void rhythm_key_on(FmDriver *d, FmdVoice *v, uint8_t drums)
{
    drums &= 0x1f;
    if (v->volume_offset & 0x80)
        drums = 0;                              /* muted */
    if (((v->note & 0x1f) | drums) == 0)
        return;
    opl(d, 0xbd, (uint8_t)((~drums & v->note) | 0x20));
    v->note |= drums;
    opl(d, 0xbd, (uint8_t)(v->note | d->rhythm_control | 0x20));
}

/* ---- song and effect control --------------------------------------------- */

static void stop_music(FmDriver *d);

/* [02cb] start_song: song n (1-based) of the song bank on voices 0-3 and the
 * rhythm voice, tempo 120. */
static void start_song(FmDriver *d, uint8_t song)
{
    uint16_t entry, track;
    int i;
    if (song == 0)
        return;
    d->current_song = song;
    d->music_ticks = 0;
    d->fade_length = 0;
    d->fade_prescaler = 0;
    d->fade_step = 0;
    d->fade_attenuation = 0;
    entry = (uint16_t)(d->music_off + 10u * (uint8_t)(song - 1));
    for (i = 0; i < 4; i++) {
        FmdVoice *v = &d->voice[i];
        clear_voice(v);
        track = peek_word(d, d->music_seg, (uint16_t)(entry + 2 * i));
        if (track)
            track = (uint16_t)(track + entry);
        v->loop = track;
        v->track = track;
    }
    {
        FmdVoice *v = &d->voice[FMD_RHYTHM];
        clear_voice(v);
        track = peek_word(d, d->music_seg, (uint16_t)(entry + 8));
        if (track)
            track = (uint16_t)(track + entry);
        v->loop = track;
        v->track = track;
    }
    d->velocity_bd = d->velocity_hh = d->velocity_sd = d->velocity_tom = d->velocity_cym = 0;
    d->rhythm_control = 0;
    opl(d, 0xbd, 0);
    set_tempo(d, 120);
}

/* [044d] key_off_effect_voices. */
static void key_off_effect_voices(FmDriver *d)
{
    voice_key_off(d, effect_voice(d, 0));
    voice_key_off(d, effect_voice(d, 1));
}

/* [046a] start_effect: effect n (1-based) of the effect bank on voices 4-5. */
static void start_effect(FmDriver *d, uint8_t effect)
{
    uint16_t entry;
    int i;
    if (effect == 0)
        return;
    d->current_effect = effect;
    entry = (uint16_t)(d->effects_off + 4u * (uint8_t)(effect - 1));
    key_off_effect_voices(d);
    for (i = 0; i < 2; i++) {
        FmdVoice *v = effect_voice(d, (unsigned)i);
        uint16_t track;
        clear_voice(v);
        v->volume = 0x0f;
        track = peek_word(d, d->effects_seg, (uint16_t)(entry + 2 * i));
        if (track)
            track = (uint16_t)(track + entry);
        v->loop = track;
        v->track = track;
    }
}

/* [092e] stop_music: silence voices 0-3 and the drums. */
static void stop_music(FmDriver *d)
{
    int i;
    d->current_song = 0;
    d->fade_length = 0;
    d->fade_attenuation = 0;
    for (i = 0; i < 4; i++) {
        voice_silence(d, &d->voice[i]);
        d->voice[i].track = 0;
    }
    rhythm_reset(d);
}

/* [0953] song_finished: the client callback, then the queued song. */
static void song_finished(FmDriver *d)
{
    FmdCallbackRegs cr;
    uint8_t song, repeat;
    cr.ax = (uint16_t)(d->queued_song | d->current_song << 8);
    cr.bx = (uint16_t)(d->queued_repeat | d->repeat_mode << 8);
    cr.cx = d->queued_repeat;
    cr.dx = (uint16_t)(d->callback_off | d->callback_seg);
    if (cr.dx != 0 && d->host.callback)
        d->host.callback(d->host.user, d->callback_seg, d->callback_off, &cr);
    song = (uint8_t)cr.ax;
    repeat = (uint8_t)cr.bx;
    d->current_song = song;
    if (song) {
        d->repeat_mode = repeat;
        start_song(d, song);
    }
    d->queued_song = (uint8_t)(cr.cx >> 8);
    d->queued_repeat = (uint8_t)cr.cx;
}

/* ---- melody tracks ------------------------------------------------------- */

/* [0a2d] D0h: mark the repeat start; D1h-DFh: repeat the section n more times. */
static uint16_t cmd_repeat(FmdVoice *v, uint16_t at, uint8_t command)
{
    uint8_t times = command & 0x0f;
    if (times == 0) {
        if (v->repeat_start == 0)
            v->repeat_start = at;
        return at;
    }
    if (v->repeat_start == 0)
        return at;
    if (v->repeat_count == 0) {
        v->repeat_count = (uint8_t)(times + 1);
        v->repeat_exit = at;
    }
    if (--v->repeat_count != 0)
        return v->repeat_start;
    v->repeat_start = 0;
    return at;
}

/* [0c3d] FDh: on the last pass, leave the repeat at once. */
static uint16_t cmd_repeat_break(FmdVoice *v, uint16_t at)
{
    if (v->repeat_count != 1)
        return at;
    v->repeat_count = 0;
    v->repeat_start = 0;
    return v->repeat_exit;
}

/* Restart a track at its loop point (end of track with repeat on). */
static uint16_t restart_track(FmdVoice *v)
{
    v->length = 0x30;
    v->gate = 0x2d;
    v->length_count = 1;
    v->gate_count = 0;
    v->repeat_count = 0;
    v->repeat_start = 0;
    return v->loop;
}

/* [0b62]/[0b6d] F7h/F6h depth, rate: software vibrato (a one-shot pitch
 * ramp after each note). `period` is the ramp length in fixed ticks. */
static void start_vibrato(FmDriver *d, FmdVoice *v, uint8_t depth, uint16_t period)
{
    v->lfo_rate = (uint16_t)(0x8000u / period);
    v->lfo_flags &= (uint8_t)~FMD_LFO_DOWN;
    if (depth & 0x80) {
        depth = (uint8_t)-depth;
        v->lfo_flags |= FMD_LFO_DOWN;
    }
    if (depth > 12)
        depth = 0;
    v->lfo_depth = modulation_depth_table[depth];
    v->lfo_count = 0;
    v->lfo_phase = 0;
    v->lfo_direction = 1;
    v->flags |= FMD_VOICE_VIBRATO;
    voice_restore_frequency(d, v);
}

/* Interpret one melody command at `at` (after the command byte); returns
 * the next track offset (0 = track ended). [0c91]..[0c54] */
static uint16_t melody_command(FmDriver *d, FmdVoice *v, uint16_t seg, uint16_t at, uint8_t command)
{
    uint8_t value;
    if (command < 0xd0)
        return at;                              /* C0-CF: no operand, ignored */
    if (command < 0xe0)
        return cmd_repeat(v, at, command);
    if (command < MEL_AM_DEPTH)
        return (uint16_t)(at + 1);              /* E0-ED: one operand, ignored */
    switch (command) {
    case MEL_AM_DEPTH:                          /* [0c0b] BDh bit 7 */
        value = peek(d, seg, at++);
        d->rhythm_control = (uint8_t)((d->rhythm_control & 0x7f) | (value & 1) << 7);
        opl(d, 0xbd, d->rhythm_control);
        return at;
    case MEL_VIBRATO_DEPTH:                     /* [0c23] BDh bit 6 */
        value = peek(d, seg, at++);
        d->rhythm_control = (uint8_t)((d->rhythm_control & 0xbf) | (value & 1) << 6);
        opl(d, 0xbd, d->rhythm_control);
        return at;
    case MEL_VOLUME:                            /* [0a5e] */
        v->volume = (uint8_t)(peek(d, seg, at++) & 0x7f);
        voice_update_volume(d, v, v->volume);
        return at;
    case MEL_TEMPO_PERIOD: {                    /* [0ae6] tempo from a period byte */
        uint16_t span = (uint16_t)(((uint8_t)~peek(d, seg, at++) + 1u) * 0x12u);
        set_tempo(d, (uint8_t)(0x1312du / span));
        return at;
    }
    case MEL_TEMPO:                             /* [0ad3] only music voices 0-3 */
        value = peek(d, seg, at++);
        if (v->opl_channel < 4 || v->opl_channel == 6)
            set_tempo(d, value);
        return at;
    case MEL_INSTRUMENT: {                      /* [0b00] relative instrument offset */
        uint16_t rel = peek_word(d, seg, at);
        at = (uint16_t)(at + 2);
        voice_set_instrument(d, v, seg, (uint16_t)(rel + at - 3));
        return at;
    }
    case MEL_VIBRATO_TEMPO: {                   /* [0b6d] period scaled by the tempo */
        uint8_t depth = peek(d, seg, at++);
        uint32_t rate = peek(d, seg, at++);
        uint16_t tempo = d->tempo ? d->tempo : 1;
        uint16_t period;
        if (rate == 0)
            rate = 0x100;
        period = (uint16_t)(rate * 0x4e2u / (uint32_t)(tempo << 4));
        start_vibrato(d, v, depth, period ? period : 1);
        return at;
    }
    case MEL_VIBRATO_RATE: {                    /* [0b62] direct period */
        uint8_t depth = peek(d, seg, at++);
        uint16_t period = (uint16_t)(peek(d, seg, at++) + 1u);
        start_vibrato(d, v, depth, period);
        return at;
    }
    case MEL_LFO_ON:                            /* [0bd8] */
        if (!(v->lfo_flags & FMD_LFO_ON)) {
            v->flags |= FMD_VOICE_LFO_WANTED;
            if (v->flags & FMD_VOICE_LFO_AVAILABLE)
                v->lfo_flags |= FMD_LFO_ON | FMD_LFO_STATE;
        }
        return at;
    case MEL_LFO_OFF:                           /* [0bed] */
        if (v->lfo_flags & FMD_LFO_ON) {
            v->flags &= (uint8_t)~FMD_VOICE_LFO_WANTED;
            v->lfo_flags &= (uint8_t)~FMD_LFO_ON;
            if (v->flags & (FMD_VOICE_KEY_ON | FMD_VOICE_SOUNDED))
                voice_restore_frequency(d, v);
        }
        return at;
    case MEL_SKIP1:
        return (uint16_t)(at + 1);
    case MEL_SKIP2:
        return (uint16_t)(at + 2);
    case MEL_REPEAT_BREAK:
        return cmd_repeat_break(v, at);
    case MEL_LOOP_POINT:                        /* [0c50] */
        v->loop = at;
        return at;
    case MEL_END:                               /* [0c54] music voices loop when repeating */
        voice_key_off(d, v);
        if (v->opl_channel >= 4 || d->repeat_mode == 0 || (v->flags & FMD_VOICE_WAITING))
            return 0;
        d->music_ticks = 0;
        return restart_track(v);
    default:                                    /* FA-FC */
        return at;
    }
}

/* [0ce0] melody_voice_read: commands until a note or the end of the track. */
static uint16_t melody_voice_read(FmDriver *d, FmdVoice *v, uint16_t seg, uint16_t at)
{
    for (;;) {
        uint8_t byte = peek(d, seg, at++);
        if (byte <= 0x5f) {                     /* note, length, gate */
            v->length = peek(d, seg, at++);
            v->gate = peek(d, seg, at++);
            voice_start_note(d, v, byte, v->gate);
            return at;
        }
        if (byte <= 0xbf) {                     /* note with the default length and gate */
            voice_start_note(d, v, (uint8_t)(byte - 0x60), v->gate);
            return at;
        }
        at = melody_command(d, v, seg, at, byte);
        if (at == 0)
            return 0;
    }
}

/* [0cbc] melody_voice_tick: count down the note and its gate. */
static void melody_voice_tick(FmDriver *d, FmdVoice *v, uint16_t seg)
{
    if (v->track == 0)
        return;
    if (--v->length_count == 0) {
        v->track = melody_voice_read(d, v, seg, v->track);
        v->flags &= (uint8_t)~FMD_VOICE_WAITING;
        return;
    }
    if (v->gate_count != 0 && --v->gate_count == 0)
        voice_key_off(d, v);
}

/* ---- rhythm track -------------------------------------------------------- */

/* One rhythm command at `at`; returns the next offset (0 = ended). [1024] table */
static uint16_t rhythm_command(FmDriver *d, FmdVoice *v, uint16_t seg, uint16_t at, uint8_t command)
{
    if (command < 0xa0) {                       /* [0ec0] 80-9F: velocity for the drums in bits 0-4 */
        rhythm_set_velocity(d, v, command, peek(d, seg, at));
        return (uint16_t)(at + 1);
    }
    if (command < 0xd0)
        return at;                              /* A0-CF: ignored */
    if (command < 0xe0)
        return cmd_repeat(v, at, command);
    if (command < RHY_VOLUME)
        return (uint16_t)(at + 1);              /* E0-EF: one operand, ignored */
    switch (command) {
    case RHY_VOLUME:                            /* [0f62] */
        rhythm_update_volume(d, v, peek(d, seg, at));
        return (uint16_t)(at + 1);
    case RHY_TEMPO:
        set_tempo(d, peek(d, seg, at));         /* the rhythm voice may set the tempo */
        return (uint16_t)(at + 1);
    case RHY_INSTRUMENTS: {                     /* [0fe3] relative rhythm set */
        uint16_t rel = peek_word(d, seg, at);
        at = (uint16_t)(at + 2);
        rhythm_load_instruments(d, seg, (uint16_t)(rel + at - 3));
        rhythm_update_volume(d, v, v->volume);
        return at;
    }
    case RHY_SKIP1_F1:
    case RHY_SKIP1_F3:
        return (uint16_t)(at + 1);
    case RHY_SKIP2_F5:
    case RHY_SKIP2_F6:
    case RHY_SKIP2_F7:
        return (uint16_t)(at + 2);
    case RHY_REPEAT_BREAK:
        return cmd_repeat_break(v, at);
    case RHY_LOOP_POINT:
        v->loop = at;
        return at;
    case RHY_END:                               /* [0ff8] */
        if (d->repeat_mode == 0 || (v->flags & FMD_VOICE_WAITING))
            return 0;
        return restart_track(v);
    default:                                    /* F8-FC */
        return at;
    }
}

/* [1063] rhythm_read: commands until a drum event. A drum event byte has
 * the drums in bits 0-4, bit 5: a length byte follows, bit 6: a velocity
 * byte follows. */
static uint16_t rhythm_read(FmDriver *d, FmdVoice *v, uint16_t seg, uint16_t at)
{
    for (;;) {
        uint8_t byte = peek(d, seg, at++);
        if (!(byte & 0x80)) {
            if (byte & 0x20)
                v->length = peek(d, seg, at++);
            if (byte & 0x40)
                rhythm_set_velocity(d, v, byte, peek(d, seg, at++));
            rhythm_key_on(d, v, byte);
            v->length_count = v->length;
            return at;
        }
        at = rhythm_command(d, v, seg, at, byte);
        if (at == 0)
            return 0;
    }
}

/* [104e] rhythm_tick. */
static void rhythm_tick(FmDriver *d, FmdVoice *v, uint16_t seg)
{
    if (v->track == 0)
        return;
    if (--v->length_count != 0)
        return;
    v->track = rhythm_read(d, v, seg, v->track);
    v->flags &= (uint8_t)~FMD_VOICE_WAITING;
}

/* ---- LFO ----------------------------------------------------------------- */

/* [10fe] lfo_step: phase += rate * direction. If the phase changes sign it
 * is left unchanged and 1 is returned. */
static int lfo_step(FmdVoice *v)
{
    uint16_t delta = (uint16_t)((int16_t)v->lfo_rate * (int16_t)v->lfo_direction);
    uint16_t old = v->lfo_phase;
    v->lfo_phase = (uint16_t)(old + delta);
    if ((old ^ v->lfo_phase) & 0x8000) {
        v->lfo_phase = old;
        return 1;
    }
    return 0;
}

/* [1117] shape 0, sawtooth: on a sign change the phase is negated (the
 * value returned is the phase before negation). */
static uint16_t lfo_sawtooth(FmdVoice *v)
{
    uint16_t value;
    int wrapped = lfo_step(v);
    value = v->lfo_phase;
    if (wrapped)
        v->lfo_phase = (uint16_t)-v->lfo_phase;
    return value;
}

/* [1120] shape 2, triangle: twice the rate; the direction reverses at the
 * top and bottom. */
static uint16_t lfo_triangle(FmdVoice *v)
{
    uint16_t delta = (uint16_t)((int16_t)(uint16_t)(v->lfo_rate << 1) * (int16_t)v->lfo_direction);
    uint16_t old = v->lfo_phase;
    v->lfo_phase = (uint16_t)(old + delta);
    if (((old ^ v->lfo_phase) & 0x8000) && ((v->lfo_direction ^ v->lfo_phase) & 0x8000)) {
        v->lfo_phase = old;
        v->lfo_direction = (uint16_t)-v->lfo_direction;
    }
    return v->lfo_phase;
}

/* [1144] shape 4 (and the software vibrato): ramp up to 7FFFh and stay. */
static uint16_t lfo_ramp(FmdVoice *v)
{
    if (v->lfo_phase == 0x7fff)
        return 0x7fff;
    if (lfo_step(v))
        v->lfo_phase = 0x7fff;
    return v->lfo_phase;
}

/* Fixed ticks per half period of the square / hold time of the random
 * shape. The original divides 7FFFh by the rate; a rate above 7FFFh would
 * make the square shape divide by zero there (a fault); it counts as 1 here. */
static uint16_t lfo_period(const FmdVoice *v)
{
    return (uint16_t)(0x7fffu / v->lfo_rate);
}

/* [1159] shape 1, square. */
static uint16_t lfo_square(FmdVoice *v)
{
    uint16_t period = lfo_period(v);
    if (period == 0)
        period = 1;
    v->lfo_phase = ((v->lfo_count / period) & 1) ? 0x8000 : 0x7fff;
    return v->lfo_phase;
}

/* [117a] shape 3, sample and hold: a new value of the generator
 * x' = x * 383h mod 7FFFh (signed) every period. */
static uint16_t lfo_random(FmDriver *d, FmdVoice *v)
{
    int16_t period = (int16_t)(lfo_period(v) | 1);
    if ((int16_t)v->lfo_count % period == 0) {
        int32_t product = (int32_t)(int16_t)d->random_state * 0x383;
        d->random_state = (uint16_t)(int16_t)(product % 0x7fff);
    }
    return d->random_state;
}

/* [11ae] lfo_sample: the instrument's shape; 8000h becomes 8001h. */
static uint16_t lfo_sample(FmDriver *d, FmdVoice *v, uint8_t shape)
{
    uint16_t value;
    switch (shape) {
    case 1: value = lfo_square(v); break;
    case 2: value = lfo_triangle(v); break;
    case 3: value = lfo_random(d, v); break;
    case 4: value = lfo_ramp(v); break;
    default: value = lfo_sawtooth(v); break;
    }
    return value == 0x8000 ? 0x8001 : value;
}

/* [11d5] voice_bend_frequency: bend (signed, full scale 7FFFh) times the
 * depth applied to the F-number; the block is renormalised upwards. The
 * resulting block/F-number is written and returned. */
static uint16_t voice_bend_frequency(FmDriver *d, FmdVoice *v, uint16_t bend)
{
    uint16_t fnum = v->frequency & 0x3ff;
    uint16_t block = v->frequency & 0x1c00;
    uint16_t value;
    if (!(bend & 0x8000)) {
        uint32_t up = (uint32_t)fnum * bend / 0x7fff;
        up = up * v->lfo_depth / 0x7fff;
        value = (uint16_t)(up + fnum);
        while (value > 0x3ff) {
            if (block >= 0x1c00) {
                block = 0x1c00;
                value = 0x3ff;
                break;
            }
            block = (uint16_t)(block + 0x400);
            value >>= 1;
        }
    } else {
        /* f * 7FFFh / (7FFFh + |bend| * depth / 7FFFh); numerator and
         * divisor halved when the divisor exceeds 16 bits. */
        uint32_t down = (uint32_t)(uint16_t)-bend * v->lfo_depth / 0x7fff;
        uint32_t numerator = (uint32_t)fnum * 0x7fff;
        uint32_t divisor = (uint16_t)down + 0x7fffu;
        if (divisor > 0xffff) {
            divisor >>= 1;
            numerator >>= 1;
        }
        value = (uint16_t)(numerator / divisor);
    }
    value |= block;
    write_frequency(d, v, value);
    return value;
}

/* [11c8] apply_pitch_modulation: LFO value times the instrument pitch
 * depth (signed byte / 7Fh). The original's 16-bit idiv faults when the
 * quotient overflows (depth -128 at full swing); the value wraps here. */
static void apply_pitch_modulation(FmDriver *d, FmdVoice *v, uint16_t value, int8_t depth)
{
    int32_t scaled = (int32_t)(int16_t)value * depth / 0x7f;
    voice_bend_frequency(d, v, (uint16_t)scaled);
}

/* [123d] apply_level_modulation: tremolo from the LFO value; instrument
 * byte 17 is the depth, bytes 18/19 scale it per operator (0-15). */
static void apply_level_modulation(FmDriver *d, FmdVoice *v, uint16_t seg, uint16_t off, uint16_t value)
{
    uint8_t shape = peek(d, seg, (uint16_t)(off + 14));
    int8_t depth = (int8_t)peek(d, seg, (uint16_t)(off + 17));
    uint16_t magnitude;
    uint16_t amount;
    int i;
    if (shape == 4) {
        value = (uint16_t)(value << 1);
    } else if (shape == 1) {
        value = (value & 0x8000) ? 0xffff : 0;
    } else if (shape == 2) {
        if (value & 0x8000)
            value = (uint16_t)-value;
        value = (uint16_t)(value << 1);
    }
    magnitude = (uint16_t)((depth < 0 ? -depth : depth) & 0x7f);
    if (depth < 0)
        value = (uint16_t)~value;
    amount = (uint16_t)((uint32_t)value * magnitude / 0xfffe);
    for (i = 0; i < 2; i++) {
        uint16_t scale = peek(d, seg, (uint16_t)(off + 18 + i)) & 0x0f;
        v->level_mod[i] = (uint8_t)((uint32_t)amount * scale / 0x0f);
        write_operator_level(d, v, i, (uint8_t)(0x40 + v->opl_operator + 3 * i), v->level[i]);
    }
}

/* [12a8] voice_lfo_tick: the instrument LFO's state machine (restart,
 * delay, run) for one fixed tick. */
static void voice_lfo_tick(FmDriver *d, FmdVoice *v, uint16_t seg, uint16_t off)
{
    uint8_t shape = peek(d, seg, (uint16_t)(off + 14));
    int run = 0;
    if (!(v->lfo_flags & FMD_LFO_ON)) {
        v->lfo_count++;
        return;
    }
    if (v->flags & (FMD_VOICE_KEY_ON | FMD_VOICE_SOUNDED)) {
        v->lfo_depth = modulation_step_table[peek(d, seg, (uint16_t)(off + 16)) & 0x0f];
        v->lfo_rate = peek_word(d, seg, (uint16_t)(off + 12));
    }
    if (!(v->flags & (FMD_VOICE_KEY_ON | FMD_VOICE_SOUNDED)) || v->lfo_rate == 0) {
        v->lfo_flags |= FMD_LFO_STATE;          /* wait for the next key-on */
        v->lfo_count++;
        return;
    }
    switch (v->lfo_flags & FMD_LFO_STATE) {
    case 0:                                     /* idle */
        break;
    case 1:                                     /* delay: count down, then start */
        if (--v->lfo_count != 0)
            return;
        v->lfo_count = 0;
        v->lfo_phase = 0;
        v->lfo_direction = 1;
        v->lfo_flags = (uint8_t)((v->lfo_flags & ~FMD_LFO_STATE) | 2);
        run = 1;
        break;
    case 2:                                     /* running */
        run = 1;
        break;
    default: {                                  /* (re)start after a key-on */
        uint8_t delay = peek(d, seg, (uint16_t)(off + 20));
        v->lfo_flags = (uint8_t)((v->lfo_flags & ~FMD_LFO_STATE) | 1);
        if (delay == 0 && shape != 4) {
            v->lfo_count = d->fixed_tick_count; /* free-running phase */
        } else if (delay <= 1) {
            v->lfo_count = 0;
            v->lfo_phase = 0;
            v->lfo_direction = 1;
        } else {
            v->lfo_count = (uint16_t)((delay - 1u) * 4u);   /* 4 fixed ticks per step after the first */
            v->lfo_count++;
            return;
        }
        v->lfo_flags = (uint8_t)((v->lfo_flags & ~FMD_LFO_STATE) | 2);
        run = 1;
        break;
    }
    }
    if (run) {
        uint16_t value = lfo_sample(d, v, shape > 4 ? 0 : shape);
        apply_pitch_modulation(d, v, value, (int8_t)peek(d, seg, (uint16_t)(off + 15)));
        apply_level_modulation(d, v, seg, off, value);
    }
    v->lfo_count++;
}

/* ---- the two tick rates -------------------------------------------------- */

/* [0e0c] effect_tick: voices 4-5; the effect ends when both tracks end. */
static void effect_tick(FmDriver *d)
{
    uint16_t any = 0;
    int i;
    if (d->current_effect == 0)
        return;
    for (i = 0; i < 2; i++) {
        FmdVoice *v = effect_voice(d, (unsigned)i);
        melody_voice_tick(d, v, d->effects_seg);
        any |= v->track;
    }
    d->effect_ticks++;
    if (any == 0) {
        d->current_effect = 0;
        d->effect_ticks = 0;
    }
}

/* [0dc7] music_tick: voices 0-3 and the rhythm voice; the song ends when
 * every track has ended. */
static void music_tick(FmDriver *d)
{
    uint16_t any = 0;
    int i;
    if (d->current_song != 0) {
        for (i = 0; i < 4; i++) {
            melody_voice_tick(d, &d->voice[i], d->music_seg);
            any |= d->voice[i].track;
        }
        rhythm_tick(d, &d->voice[FMD_RHYTHM], d->music_seg);
        any |= d->voice[FMD_RHYTHM].track;
        d->music_ticks++;
        if (any != 0)
            return;
        song_finished(d);
        if (d->current_song != 0)
            return;
    }
    d->tempo_increment = 0;                     /* no song: the music clock stops */
    d->tempo = 0;
}

/* [133e] fixed_tick (about 62 Hz): fade-out, effects, vibrato and LFOs. */
static void fixed_tick(FmDriver *d)
{
    int i;
    if (d->fade_length != 0) {
        uint8_t prescaler = (uint8_t)(d->fade_prescaler + 1);
        if (prescaler >= 4) {
            uint8_t length = d->fade_length;
            uint8_t step = d->fade_step;
            uint32_t attenuation;
            if (step >= length) {               /* faded out: stop, next song */
                stop_music(d);
                song_finished(d);
                d->fixed_tick_count++;
                return;
            }
            step++;
            d->fade_step = step;
            /* quadratic curve: FFh * (step / length)^2 */
            attenuation = (uint16_t)(step * step) * 0xffu / (uint16_t)(length * length);
            if (attenuation > 0xff)
                attenuation = 0xff;
            if ((uint8_t)attenuation != d->fade_attenuation) {
                d->fade_attenuation = (uint8_t)attenuation;
                for (i = 0; i < 4; i++)
                    voice_update_volume(d, &d->voice[i], d->voice[i].volume);
                rhythm_update_volume(d, &d->voice[FMD_RHYTHM], d->voice[FMD_RHYTHM].volume);
            }
            prescaler = 0;
        }
        d->fade_prescaler = prescaler;
    }
    effect_tick(d);
    for (i = 0; i < FMD_VOICES; i++) {
        FmdVoice *v = &d->voice[i];
        if (v->lfo_flags & FMD_LFO_RAMP) {      /* software vibrato */
            uint16_t bend = lfo_ramp(v);
            uint16_t frequency;
            if (v->lfo_flags & FMD_LFO_DOWN)
                bend = (uint16_t)-bend;
            frequency = voice_bend_frequency(d, v, bend);
            if (v->lfo_phase == 0x7fff) {       /* ramp done: keep the bent pitch */
                v->frequency = frequency;
                v->lfo_flags &= (uint8_t)~FMD_LFO_RAMP;
            }
        } else if (v->instrument_seg | v->instrument_off) {
            voice_lfo_tick(d, v, v->instrument_seg, v->instrument_off);
        }
    }
    d->fixed_tick_count++;
}

/* [09c9] update_sequencer: run the ticks the timer accumulated. */
static void update_sequencer(FmDriver *d)
{
    d->busy++;
    do {
        if (d->pending_music_ticks) {
            music_tick(d);
            d->pending_music_ticks--;
        }
        if (d->pending_fixed_ticks) {
            fixed_tick(d);
            d->pending_fixed_ticks--;
        }
    } while (d->pending_music_ticks | d->pending_fixed_ticks);
    d->busy--;
}

/* [01de] the PIT interrupt. Music ticks come from the tempo accumulator,
 * fixed ticks at 36h/256 of the PIT rate (62.1 Hz), the two countdowns at
 * 35h/256 (60.3 Hz). Every 16th PIT tick belongs to the BIOS clock (18.2
 * Hz) and does not sequence; its ticks run on the next one. */
void fmd_timer_tick(FmDriver *d)
{
    int n;
    uint32_t sum = (uint32_t)d->tempo_accumulator + d->tempo_increment;
    d->tempo_accumulator = (uint16_t)sum;
    if (sum > 0xffff)
        d->pending_music_ticks++;
    if (d->fixed_tick_accumulator + 0x36u > 0xff)
        d->pending_fixed_ticks++;
    d->fixed_tick_accumulator = (uint8_t)(d->fixed_tick_accumulator + 0x36);
    for (n = 0; n < 2; n++) {
        if (d->countdown_accumulator[n] + 0x35u > 0xff && d->countdown[n] != 0)
            d->countdown[n]--;
        d->countdown_accumulator[n] = (uint8_t)(d->countdown_accumulator[n] + 0x35);
    }
    if (--d->bios_tick_divider == 0) {
        d->bios_tick_divider = 16;
        return;
    }
    if (d->busy == 0 && (d->pending_music_ticks | d->pending_fixed_ticks) && d->enabled)
        update_sequencer(d);
}

/* ---- the API ------------------------------------------------------------- */

static void return_status(const FmDriver *d, FmdRegs *r)
{
    r->ax = (uint16_t)(d->current_song | d->repeat_mode << 8);
}

/* 02h: AL = 0 stops at once, otherwise fades out over AL steps. */
static void api_stop_music(FmDriver *d, uint8_t speed, FmdRegs *r)
{
    d->queued_song = 0;
    if (speed == 0)
        stop_music(d);
    else
        d->fade_length = speed;
    return_status(d, r);
}

/* 12h */
static void api_stop_effect(FmDriver *d)
{
    d->current_effect = 0;
    key_off_effect_voices(d);
    voice_silence(d, effect_voice(d, 0));
    voice_silence(d, effect_voice(d, 1));
    d->effect_ticks = 0;
}

/* 00h: song bank at DS:DX. */
static void api_load_music(FmDriver *d, FmdRegs *r)
{
    int i;
    api_stop_music(d, 0, r);
    d->music_seg = r->ds;
    d->music_off = r->dx;
    for (i = 0; i < 4; i++) {
        d->voice[i].volume_offset = 0;
        d->voice[i].transpose = 0;
    }
    d->voice[FMD_RHYTHM].volume_offset = 0;
    d->voice[FMD_RHYTHM].transpose = 0;
    r->ax = 0;
}

/* 10h: effect bank at DS:DX. */
static void api_load_effects(FmDriver *d, FmdRegs *r)
{
    int i;
    api_stop_effect(d);
    d->effects_seg = r->ds;
    d->effects_off = r->dx;
    d->effect_ticks = 0;
    key_off_effect_voices(d);
    for (i = 0; i < 2; i++) {
        effect_voice(d, (unsigned)i)->volume_offset = 0x0f;
        effect_voice(d, (unsigned)i)->transpose = 0;
    }
    r->ax = 0;
}

/* 05h: effect voice AL gets the instrument at DS:DX (NULL silences it). */
static void api_set_voice_instrument(FmDriver *d, uint8_t n, FmdRegs *r)
{
    if (n < 2) {
        FmdVoice *v = effect_voice(d, n);
        voice_silence(d, v);
        if (!(r->ds | r->dx)) {
            clear_voice(v);
            opl(d, (uint8_t)(0x40 + v->opl_operator), 0x3f);
            v->ksl_tl[0] = 0x3f;
            opl(d, (uint8_t)(0x43 + v->opl_operator), 0x3f);
            v->ksl_tl[1] = 0x3f;
        } else {
            v->volume = 0;
            v->flags |= FMD_VOICE_LFO_WANTED;
            voice_set_instrument(d, v, r->ds, r->dx);
        }
    }
    r->ax = 0;
}

/* 06h: note DL (0-5Fh) on effect voice AL. */
static void api_play_voice_note(FmDriver *d, uint8_t n, FmdRegs *r)
{
    uint8_t note = (uint8_t)r->dx;
    if (n < 2 && note <= 0x5f) {
        FmdVoice *v = effect_voice(d, n);
        v->flags &= 0x9d;                       /* not legato, no vibrato, not sounded */
        voice_start_note(d, v, note, 1);
        v->flags &= (uint8_t)~(FMD_VOICE_LEGATO | FMD_VOICE_VIBRATO);
    }
    r->ax = 0;
}

/* 80h: volume offset DL for voice AL (0-5 melody, 6 rhythm). */
static void api_set_voice_volume(FmDriver *d, uint8_t n, uint8_t offset)
{
    FmdVoice *v;
    if (n > 6)
        return;
    v = &d->voice[n];
    v->volume_offset = offset;
    if (n == 6)
        rhythm_update_volume(d, v, v->volume);
    else
        voice_update_volume(d, v, v->volume);
}

/* 82h: rhythm set at DS:DX (NULL resets rhythm mode). */
static void api_set_rhythm_instruments(FmDriver *d, FmdRegs *r)
{
    FmdVoice *v = &d->voice[FMD_RHYTHM];
    if (!(r->ds | r->dx)) {
        rhythm_reset(d);
        return;
    }
    rhythm_load_instruments(d, r->ds, r->dx);
    rhythm_update_volume(d, v, v->volume);
}

/* 83h: key the drums in DL; AL bit 7 clear: AL is their velocity. */
static void api_rhythm_key(FmDriver *d, uint8_t velocity, uint8_t drums)
{
    FmdVoice *v = &d->voice[FMD_RHYTHM];
    if (!(velocity & 0x80))
        rhythm_set_velocity(d, v, drums, velocity);
    rhythm_key_on(d, v, drums);
}

/* [0277] dispatch_api: functions 00-09h, 10-13h, 80-83h. Registers not
 * named as results come back unchanged. */
static void dispatch(FmDriver *d, FmdRegs *r)
{
    uint8_t function = (uint8_t)(r->ax >> 8), arg = (uint8_t)r->ax;
    uint16_t swap;
    if (!(function <= 0x09 || (function >= 0x10 && function <= 0x13) || (function >= 0x80 && function <= 0x83)))
        return;
    d->busy++;
    switch (function) {
    case 0x00: api_load_music(d, r); break;
    case 0x01:                                  /* play song AL */
        api_stop_music(d, 0, r);
        if (d->music_seg | d->music_off)
            start_song(d, arg);
        return_status(d, r);
        break;
    case 0x02: api_stop_music(d, arg, r); break;
    case 0x03:                                  /* BX = music ticks */
        r->bx = d->music_ticks;
        return_status(d, r);
        break;
    case 0x04:                                  /* repeat mode */
        d->repeat_mode = arg;
        return_status(d, r);
        break;
    case 0x05: api_set_voice_instrument(d, arg, r); break;
    case 0x06: api_play_voice_note(d, arg, r); break;
    case 0x07:                                  /* key off effect voice AL */
        if (arg < 2)
            voice_key_off(d, effect_voice(d, arg));
        r->ax = 0;
        break;
    case 0x08:                                  /* queue song AL with repeat mode DL */
        d->queued_song = arg;
        d->queued_repeat = (uint8_t)r->dx;
        return_status(d, r);
        break;
    case 0x09:                                  /* swap the end-of-song callback */
        swap = d->callback_seg;
        d->callback_seg = r->ds;
        r->ds = swap;
        swap = d->callback_off;
        d->callback_off = r->dx;
        r->dx = swap;
        break;
    case 0x10: api_load_effects(d, r); break;
    case 0x11:                                  /* play effect AL */
        api_stop_effect(d);
        start_effect(d, arg);
        break;
    case 0x12: api_stop_effect(d); break;
    case 0x13:                                  /* AL = effect, BX = effect ticks */
        r->ax = (uint16_t)((r->ax & 0xff00) | d->current_effect);
        r->bx = d->effect_ticks;
        break;
    case 0x80: api_set_voice_volume(d, arg, (uint8_t)r->dx); break;
    case 0x81:                                  /* transpose DL for melody voice AL */
        if (arg < 6)
            d->voice[arg].transpose = (uint8_t)r->dx;
        break;
    case 0x82: api_set_rhythm_instruments(d, r); break;
    case 0x83: api_rhythm_key(d, arg, (uint8_t)r->dx); break;
    }
    d->busy--;
}

void fmd_far_call(FmDriver *d, FmdRegs *r)
{
    if (!d->enabled) {
        r->ax = 0xffff;
        return;
    }
    dispatch(d, r);
}

void fmd_interrupt(FmDriver *d, FmdRegs *r)
{
    uint8_t function = (uint8_t)(r->ax >> 8), arg = (uint8_t)r->ax;
    switch (function) {
    case 0xff:
        fmd_uninstall(d);
        return;
    case 0xfe:                                  /* version 3.3, AH = enabled */
        r->ax = (uint16_t)(d->enabled << 8 | 0x33);
        return;
    case 0xf0:
    case 0xf1:                                  /* countdowns: AL = 0 read, else set */
        if (arg == 0) {
            r->bx = d->countdown[function & 1];
        } else {
            d->countdown[function & 1] = r->bx;
            d->countdown_accumulator[function & 1] = 0;
        }
        return;
    default:
        fmd_far_call(d, r);
        return;
    }
}

/* [1402] initialize_opl: waveform select on, note select, no rhythm;
 * every voice cleared and silenced. */
static void initialize_chip(FmDriver *d)
{
    int i;
    opl(d, 0x01, 0x20);
    opl(d, 0x08, 0x40);
    opl(d, 0xbd, 0x00);
    for (i = 0; i < FMD_VOICES; i++) {
        clear_voice(&d->voice[i]);
        voice_silence(d, &d->voice[i]);
    }
    clear_voice(&d->voice[FMD_RHYTHM]);
    rhythm_reset(d);
}

/* [14c6] detect_opl: the AdLib timer test (timer 1 must overflow). */
static int detect_chip(FmDriver *d)
{
    uint8_t before, after;
    opl(d, 0x04, 0x60);
    opl(d, 0x04, 0x80);
    before = d->host.read_status ? d->host.read_status(d->host.user) : 0;
    opl(d, 0x02, 0xff);
    opl(d, 0x04, 0x21);
    /* the original waits here (30000 loop iterations, > 80 us) */
    after = d->host.read_status ? d->host.read_status(d->host.user) : 0;
    opl(d, 0x04, 0x60);
    opl(d, 0x04, 0x80);
    return (before & 0xe0) == 0 && (after & 0xe0) == 0xc0;
}

int fmd_install(FmDriver *d, const FmdHost *host, int detect)
{
    int i;
    memset(d, 0, sizeof *d);
    d->host = *host;
    for (i = 0; i <= FMD_VOICES; i++) {
        d->voice[i].opl_channel = voice_channel[i];
        d->voice[i].opl_operator = voice_operator[i];
    }
    d->bios_tick_divider = 1;
    d->random_state = 0x037d;
    d->tempo = 120;
    if (detect && !detect_chip(d))
        return 0;                               /* resident without a sequencer */
    d->enabled = 0xff;
    d->busy++;
    initialize_chip(d);
    d->busy--;
    return 1;
}

/* [01a5] uninstall_driver: the chip is reset; the sequencer stays blocked. */
void fmd_uninstall(FmDriver *d)
{
    if (!d->enabled)
        return;
    d->busy++;
    initialize_chip(d);
}
