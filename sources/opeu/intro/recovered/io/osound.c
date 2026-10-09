/* OPEN 0349:0002..02b1 (image 0x3492..0x3742): INT 66h music and sound-effect
 * driver interface, the OPEN build of MAIN's SOUND (0d65, src/main/io/sound.c),
 * whose C text is copied here; the routines that differ are marked OPEN.
 * DS:1c20 holds the driver-open result, DS:1c2e the sound-enabled flag,
 * DS:0182/0183 the current music track/effect, and DS:0184/019e the 7- and
 * 13-byte music bank/path buffers. Music commands are AH=1 start, 2 stop,
 * 3 status, 4 control, 5 load, 6 select, 7; effect commands are AH=10h load,
 * 11h start, 12h stop, 13h status. The REGS/SREGS layouts follow Microsoft
 * C's <dos.h>. OPEN has no sound reset or effect finish; the driver is opened
 * with the FM driver image at 0710:0002 only (0000:13e0).
 */
#include "types.h"

typedef struct { uint16_t ax, bx, cx, dx, si, di, cflag; } WordRegs;
typedef struct { uint8_t al, ah, bl, bh, cl, ch, dl, dh; } ByteRegs;
typedef union { WordRegs x; ByteRegs h; } Regs;
typedef struct { uint16_t es, cs, ss, ds; } SegRegs;

#if defined(_MSC_VER) && _MSC_VER < 1000
/* Unused declarations (no EXTDEF): C 6.0a orders the operands of the
 * `MUSIC_CURRENT == track` and `EFFECT_CURRENT == effect` tests by identifier
 * number. Two names here and one before the opeu_effect_start prototype give
 * OPEN's `mov al,[current]; cmp [bp-n],al` in both routines (a scratch sweep
 * of 0..16 names at both places found this as the first exact fit). */
extern int opeu_sound_unused1, opeu_sound_unused2;
extern int opeu_sound_driver, opeu_sound_enabled;
extern uint8_t opeu_music_current, opeu_effect_current;
extern char opeu_music_bank[], opeu_music_path[];
extern char far opeu_music_tracks[];
/* C library 0000:1f40, 0000:1fc0, 0000:2056; driver open 0000:13e0. */
extern int _cdecl int86(int number, Regs *in, Regs *out);
extern int _cdecl int86x(int number, Regs *in, Regs *out, SegRegs *segments);
extern void _cdecl segread(SegRegs *segments);
extern int _cdecl opeu_open_sound_open(char far *driver);
extern void opeu_copy_bytes(const void *source, void *destination, uint16_t size);
void opeu_sound_initialize(void);
void opeu_finish_side_initialization(uint8_t value);
void opeu_music_stop(void);
int opeu_music_playing(void);
void opeu_music_start(uint8_t track);
void opeu_set_turn_context(uint8_t track);
void opeu_music_load(char *bank, char *path);
void opeu_music_select(uint8_t value);
void opeu_music_load_select(char *bank, char *path, uint8_t value);
void opeu_music_command7(void);
void opeu_effect_stop(void);
int opeu_effect_playing(void);
void opeu_effect_load(char far *data);
extern int opeu_sound_unused3;
void opeu_effect_start(uint8_t effect);
void opeu_effect_play(uint8_t effect);
#pragma alloc_text(C0349_SND, opeu_sound_initialize, opeu_finish_side_initialization, opeu_music_stop, opeu_music_playing, opeu_music_start, opeu_set_turn_context, opeu_music_load, opeu_music_select, opeu_music_load_select, opeu_music_command7, opeu_effect_stop, opeu_effect_playing, opeu_effect_load, opeu_effect_start, opeu_effect_play)
#define STATE void
#define STATE_
#define WITH
#define WITH_
#define NEAR char *
#define FAR char far *
#define REGS Regs r
#define FRAME_PAD long state; /* unused: the original frame reserves four bytes above r */
#define DRIVER opeu_sound_driver
#define ENABLED opeu_sound_enabled
#define OPEN() (opeu_sound_driver = opeu_open_sound_open(opeu_music_tracks))
#define SET_ENABLED(v) (opeu_sound_enabled = (v))
#define MUSIC_CURRENT opeu_music_current
#define EFFECT_CURRENT opeu_effect_current
#define MUSIC_BANK opeu_music_bank
#define MUSIC_PATH opeu_music_path
#define BANK_OFFSET (uint16_t)opeu_music_bank
#define COPY(s, d, n) opeu_copy_bytes(s, d, n)
#define INT86(r) int86(0x66, &r, &r)
#define INT86X(r, s) int86x(0x66, &r, &r, &s)
#define SEGREAD(s) segread(&s)
#else
/* Host model: ds is the original 64 KiB data segment; near pointers are DS
 * offsets and far pointers are segment:offset values relative to the load
 * segment. The service is the INT 66h boundary: call 0 is the driver open
 * routine 0000:13e0 (words: driver image offset/segment), call 1 is
 * int86 (seven REGS words in/out), call 2 int86x (REGS then four SREGS
 * words), call 3 segread (four SREGS words out). The original passes the
 * same REGS as input and output; unassigned fields are zero here.
 */
typedef uint16_t (*SoundService)(uint8_t *ds, uint16_t call, uint16_t *words);
#define STATE uint8_t ds[65536], SoundService service
#define STATE_ STATE,
#define WITH ds, service
#define WITH_ ds, service,
#define NEAR uint16_t
#define FAR uint32_t
#define REGS Regs r = {{0, 0, 0, 0, 0, 0, 0}}
#define FRAME_PAD
#define WORD_AT(n) ((uint16_t)(ds[n] | (uint16_t)ds[(n)+1] << 8))
#define DRIVER WORD_AT(0x1c20)
#define ENABLED WORD_AT(0x1c2e)
#define OPEN() host_set_word(ds, 0x1c20, host_open(ds, service))
#define SET_ENABLED(v) host_set_word(ds, 0x1c2e, (uint16_t)(v))
#define MUSIC_CURRENT ds[0x182]
#define EFFECT_CURRENT ds[0x183]
#define MUSIC_BANK 0x184
#define MUSIC_PATH 0x19e
#define BANK_OFFSET 0x184
#define COPY(s, d, n) host_copy(ds, s, d, n)
#define INT86(r) host_int86(ds, service, &r, 0)
#define INT86X(r, s) host_int86(ds, service, &r, &s)
#define SEGREAD(s) host_segread(ds, service, &s)
int opeu_music_playing(STATE);
int opeu_effect_playing(STATE);
void opeu_music_stop(STATE);
void opeu_effect_stop(STATE);
void opeu_music_start(STATE_ uint8_t track);
void opeu_effect_start(STATE_ uint8_t effect);
void opeu_finish_side_initialization(STATE_ uint8_t value);
void opeu_music_load(STATE_ NEAR bank, NEAR path);
void opeu_music_select(STATE_ uint8_t value);

static void host_set_word(uint8_t *ds, uint16_t at, uint16_t value)
{
    ds[at] = (uint8_t)value;
    ds[(uint16_t)(at+1)] = (uint8_t)(value >> 8);
}

static uint16_t host_open(uint8_t *ds, SoundService service)
{
    uint16_t words[2] = {0x0002, 0x0710};
    return service(ds, 0, words);
}

/* 09cc:000a copies forward when destination < source, else backward;
 * both near pointers wrap within DS. */
static void host_copy(uint8_t *ds, uint16_t source, uint16_t destination, uint16_t size)
{
    uint16_t i;
    if (destination < source)
        for (i = 0; i < size; i++)
            ds[(uint16_t)(destination+i)] = ds[(uint16_t)(source+i)];
    else
        for (i = size; i; i--)
            ds[(uint16_t)(destination+i-1)] = ds[(uint16_t)(source+i-1)];
}

static uint16_t host_int86(uint8_t *ds, SoundService service, Regs *r, SegRegs *s)
{
    uint16_t words[11], result;
    words[0] = r->x.ax; words[1] = r->x.bx; words[2] = r->x.cx; words[3] = r->x.dx;
    words[4] = r->x.si; words[5] = r->x.di; words[6] = r->x.cflag;
    if (s) {
        words[7] = s->es; words[8] = s->cs; words[9] = s->ss; words[10] = s->ds;
    }
    result = service(ds, s ? 2 : 1, words);
    r->x.ax = words[0]; r->x.bx = words[1]; r->x.cx = words[2]; r->x.dx = words[3];
    r->x.si = words[4]; r->x.di = words[5]; r->x.cflag = words[6];
    if (s) {
        s->es = words[7]; s->cs = words[8]; s->ss = words[9]; s->ds = words[10];
    }
    return result;
}

static void host_segread(uint8_t *ds, SoundService service, SegRegs *s)
{
    uint16_t words[4] = {0, 0, 0, 0};
    service(ds, 3, words);
    s->es = words[0]; s->cs = words[1]; s->ss = words[2]; s->ds = words[3];
}
#endif

void opeu_sound_initialize(STATE)
{
    OPEN();
    SET_ENABLED(DRIVER != 0);
}

void opeu_finish_side_initialization(STATE_ uint8_t value)
{
    REGS;
    if (DRIVER && ENABLED) {
        r.h.ah = 4;
        r.h.al = value;
        INT86(r);
    }
}

/* OPEN: no driver test. */
void opeu_music_stop(STATE)
{
    REGS;
    r.h.ah = 2;
    r.h.al = 20;
    INT86(r);
}

/* OPEN: tests the driver like opeu_effect_playing. */
int opeu_music_playing(STATE)
{
    int state;
    REGS;
    if (!DRIVER)
        return 0;
    r.h.ah = 3;
    state = INT86(r);
    return (state & 0xff) != 0;
}

void opeu_music_start(STATE_ uint8_t track)
{
    REGS;
    if (!DRIVER)
        return;
    if (opeu_music_playing(WITH)) {
        if (MUSIC_CURRENT == track)
            return;
        opeu_music_stop(WITH);
        while (opeu_music_playing(WITH))
            ;
    }
    r.h.ah = 1;
    r.h.al = track + 1;
    INT86(r);
    MUSIC_CURRENT = track;
}

void opeu_set_turn_context(STATE_ uint8_t track)
{
    if (ENABLED) {
        opeu_music_start(WITH_ track);
        opeu_finish_side_initialization(WITH_ 1);
    }
}

void opeu_music_load(STATE_ NEAR bank, NEAR path)
{
    REGS;
    SegRegs segs;
    if (!DRIVER)
        return;
    COPY(bank, MUSIC_BANK, 7);
    COPY(path, MUSIC_PATH, 13);
    SEGREAD(segs);
    r.h.al = 0;
    r.x.dx = BANK_OFFSET;
    r.h.ah = 5;
    INT86X(r, segs);
}

void opeu_music_select(STATE_ uint8_t value)
{
    REGS;
    if (DRIVER) {
        r.h.al = 0;
        r.h.dl = value;
        r.h.ah = 6;
        INT86(r);
    }
}

void opeu_music_load_select(STATE_ NEAR bank, NEAR path, uint8_t value)
{
    if (!ENABLED)
        return;
    opeu_music_load(WITH_ bank, path);
    opeu_music_select(WITH_ value);
}

/* OPEN: tests the driver. */
void opeu_music_command7(STATE)
{
    REGS;
    if (DRIVER) {
        r.h.al = 0;
        r.h.ah = 7;
        INT86(r);
    }
}

void opeu_effect_stop(STATE)
{
    REGS;
    if (DRIVER) {
        r.h.ah = 18;
        INT86(r);
    }
}

int opeu_effect_playing(STATE)
{
    int state;
    REGS;
    if (!DRIVER)
        return 0;
    r.h.ah = 19;
    state = INT86(r);
    return (state & 0xff) != 0;
}

void opeu_effect_load(STATE_ FAR data)
{
    REGS;
    SegRegs segs;
    if (DRIVER) {
        r.h.ah = 16;
        segs.ds = (uint16_t)((uint32_t)data >> 16);
        r.x.dx = (uint16_t)(uint32_t)data;
        INT86X(r, segs);
    }
}

void opeu_effect_start(STATE_ uint8_t effect)
{
    REGS;
    FRAME_PAD
    if (!DRIVER)
        return;
    if (opeu_effect_playing(WITH)) {
        if (EFFECT_CURRENT == effect)
            return;
        opeu_effect_stop(WITH);
        while (opeu_effect_playing(WITH))
            ;
    }
    /* OPEN: always starts effect slot 1 and records the requested effect. */
    r.h.ah = 17;
    r.h.al = 1;
    INT86(r);
    EFFECT_CURRENT = effect;
}

void opeu_effect_play(STATE_ uint8_t effect)
{
    if (ENABLED)
        opeu_effect_start(WITH_ effect);
}

