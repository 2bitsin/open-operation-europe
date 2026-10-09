/* OPEN 062d:0008..041b (image 0x62d8..0x66ec): the opening's helper
 * segment. Its routines come from several MAIN files; the ones that are
 * byte-identical in OPEN keep MAIN's C text: the mouse services (MOUSE 05a8,
 * src/main/io/mouse.c), the picture/outline helpers (UIPRIM 0a04,
 * src/main/core/uiprim.c) and the disk reset (RESOPEN 0977,
 * src/main/io/resopen.c). The rest is OPEN's: a tick wait with input abort,
 * the image-stream variants that always read the picture file DS:0440, the
 * palette set/step/fade helpers on the current palette DS:0358, the fade
 * wrappers, the input poll, the open-or-die file opener, the text clear and
 * three empty routines the text engine calls.
 *
 * DS:0356 stream byte count, DS:0358 current palette (16 RGB triples of
 * 0..15), DS:0388 black palette, DS:1944 the alternative fade target,
 * DS:0408 "OPEN ERROR=  %s", DS:0440 picture file handle, DS:1c0e far
 * stream pointer, DS:1c30 image buffer segment.
 */
#include "types.hpp"

typedef struct {
  uint16_t ax, bx, cx, dx, si, di, cflag;
} WordRegs;
typedef struct {
  uint8_t al, ah, bl, bh, cl, ch, dl, dh;
} ByteRegs;
typedef union {
  WordRegs x;
  ByteRegs h;
} Regs;

/* Host model: ds is DGROUP; near pointers are DS offsets except the palette
 * pointers, which are host pointers into ds. Every routine outside this
 * segment is a scripted service event(ds, id, words, count) returning DX:AX:
 *  0 fm_busy()                       9 vga_clear()
 *  1 int86(0x33) REGS (7 words io)  10 turn_input_b()
 *  2 int86(0x21) (number, AH)       11 input_flush()
 *  3 dos_read_far(h, seg, off, n)   12 cursor_print(format, name)
 *  4 dos_seek(h, lo, hi, origin)    13 the open-error spin: nonzero ends it
 *  5 set_palette(first, count, p)   14 text_put(c)
 *  6 identity_palette()             15 open_delay(ticks)
 *  7 ui_line(x1, y1, x2, y2, c)     16 far byte read (seg, off)
 *  8 picture_decode_masked(x, y, reader offset, reader segment)
 *  17 dos_open(name, mode)
 * The original spins forever after an open error; the host asks event 13
 * on each turn so a caller can end the spin (the routine then returns -1).
 */
typedef uint32_t (*UtilService)(uint8_t* ds, uint16_t id, uint16_t* words, uint16_t count);
int16_t opeu_open_wait_ticks(uint8_t ds[65536], UtilService event, uint16_t ticks);
void opeu_mouse_window(uint8_t ds[65536], UtilService event, int16_t x, int16_t y, int16_t width, int16_t height);
uint16_t opeu_mouse_reset(uint8_t ds[65536], UtilService event);
int opeu_mouse_left(uint8_t ds[65536], UtilService event);
int opeu_mouse_right(uint8_t ds[65536], UtilService event);
int opeu_turn_input_a(uint8_t ds[65536], UtilService event);
uint8_t opeu_stream_file_byte(uint8_t ds[65536], UtilService event);
void opeu_stream_seek(uint8_t ds[65536], UtilService event, int32_t position);
void opeu_picture_file_at(uint8_t ds[65536], UtilService event, uint16_t x, uint16_t y, int32_t position);
void opeu_open_palette_set(uint8_t ds[65536], UtilService event, uint8_t* palette);
int16_t opeu_open_palette_step(uint8_t ds[65536], UtilService event, uint8_t* target);
int16_t opeu_open_palette_fade(uint8_t ds[65536], UtilService event, uint8_t* target, uint16_t mode);
int16_t opeu_open_fade_in(uint8_t ds[65536], UtilService event);
int16_t opeu_open_fade_in_wait(uint8_t ds[65536], UtilService event);
int16_t opeu_open_fade_alt(uint8_t ds[65536], UtilService event);
void opeu_open_identity_palette(uint8_t ds[65536], UtilService event);
int16_t opeu_open_input_poll(uint8_t ds[65536], UtilService event);
int16_t opeu_open_file_or_die(uint8_t ds[65536], UtilService event, uint16_t name);
void opeu_disk_reset(uint8_t ds[65536], UtilService event);
void opeu_open_text_clear(uint8_t ds[65536], UtilService event);
void opeu_ui_outline(uint8_t ds[65536], UtilService event, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                     uint16_t color);
void opeu_open_stub_a(uint8_t ds[65536], UtilService event);
void opeu_open_stub_b(uint8_t ds[65536], UtilService event);
void opeu_open_stub_c(uint8_t ds[65536], UtilService event);
#define HOST_WORD(n) (*(uint16_t*)(ds + (uint16_t)(n)))
static uint32_t util_call(uint8_t* ds, UtilService event, uint16_t id, uint16_t count, uint16_t a, uint16_t b,
                          uint16_t c, uint16_t d, uint16_t e) {
  uint16_t words[5];
  words[0] = a;
  words[1] = b;
  words[2] = c;
  words[3] = d;
  words[4] = e;
  return event(ds, id, words, count);
}
static void util_int86(uint8_t* ds, UtilService event, Regs* r) {
  uint16_t words[7];
  words[0] = r->x.ax;
  words[1] = r->x.bx;
  words[2] = r->x.cx;
  words[3] = r->x.dx;
  words[4] = r->x.si;
  words[5] = r->x.di;
  words[6] = r->x.cflag;
  event(ds, 1, words, 7);
  r->x.ax = words[0];
  r->x.bx = words[1];
  r->x.cx = words[2];
  r->x.dx = words[3];
  r->x.si = words[4];
  r->x.di = words[5];
  r->x.cflag = words[6];
}
/* *opeu_stream_ptr++ through the far pointer DS:1c0e (offset), 1c10 (segment). */
static uint8_t util_stream_next(uint8_t* ds, UtilService event) {
  uint16_t offset = HOST_WORD(0x1c0e);
  HOST_WORD(0x1c0e) = (uint16_t)(offset + 1);
  return (uint8_t)util_call(ds, event, 16, 2, HOST_WORD(0x1c10), offset, 0, 0, 0);
}
#define STATE uint8_t ds[65536], UtilService event
#define STATE_ STATE,
#define WITH ds, event
#define WITH_ ds, event,
#define NOARGS STATE
#define ARGS WITH_
#define REGS Regs r = { { 0, 0, 0, 0, 0, 0, 0 } }
#define INT86(r) util_int86(ds, event, &r)
#define LOCALS
#define DISK_RESET() util_call(ds, event, 2, 2, 0x21, 0x0d, 0, 0, 0)
#define PAL uint8_t*
#define NAME uint16_t
#define UNUSED() ((void)ds, (void)event)
#define TICKS() (uint16_t)util_call(ds, event, 0, 0, 0, 0, 0, 0, 0)
#define POLL() opeu_open_input_poll(ds, event)
#define STREAM_COUNT HOST_WORD(0x356)
#define PICTURE_HANDLE HOST_WORD(0x440)
#define IMAGE_SEG HOST_WORD(0x1c30)
#define STREAM_SEG HOST_WORD(0x1c10)
#define STREAM_OFF HOST_WORD(0x1c0e)
#define STREAM_NEXT() util_stream_next(ds, event)
#define FILE_READER 0x132
#define CURRENT (ds + 0x358)
#define BLACK (ds + 0x388)
#define FADE_TARGET (ds + 0x1944)
#define SEEK(h, p, o) \
  (int32_t)util_call(ds, event, 4, 4, (uint16_t)(h), (uint16_t)(p), (uint16_t)((uint32_t)(p) >> 16), o, 0)
#define READ_FAR(h, s, o, c) (uint16_t)util_call(ds, event, 3, 4, (uint16_t)(h), s, o, c, 0)
#define LINE(a, b, c, d, e) util_call(ds, event, 7, 5, a, b, c, d, e)
#define SET_PALETTE(a, b, c) util_call(ds, event, 5, 3, a, b, (uint16_t)((c) - ds), 0, 0)
#define DECODE_MASKED(x, y, r) util_call(ds, event, 8, 4, x, y, r, 0x062d, 0)
#define IDENTITY_PALETTE() util_call(ds, event, 6, 0, 0, 0, 0, 0, 0)
#define INPUT_B() (int16_t)util_call(ds, event, 10, 0, 0, 0, 0, 0, 0)
#define FLUSH() util_call(ds, event, 11, 0, 0, 0, 0, 0, 0)
#define DELAY(n) util_call(ds, event, 15, 1, n, 0, 0, 0, 0)
#define OPEN(name, mode) (int16_t)util_call(ds, event, 17, 2, name, mode, 0, 0, 0)
#define CLEAR() util_call(ds, event, 9, 0, 0, 0, 0, 0, 0)
#define ERROR_PRINT(name) util_call(ds, event, 12, 2, 0x408, name, 0, 0, 0)
#define HANG() while (!util_call(ds, event, 13, 0, 0, 0, 0, 0, 0))
#define TEXT_PUT(c) util_call(ds, event, 14, 1, c, 0, 0, 0, 0)

/* 062d:0008. AX=ticks. Waits until the FM driver's down-counter has run
 * `ticks` further; 1 if the input poll reported a key or button first. */
int16_t opeu_open_wait_ticks(STATE_ uint16_t ticks) {
  uint16_t target;
  target = TICKS();
  target -= ticks;
  do {
    if (POLL()) return 1;
  } while (TICKS() > target);
  return 0;
}

/* 062d:003e (MAIN 05a8:0210). AX=x, DX=y, BX=width, stack height; RETF 2. */
void opeu_mouse_window(STATE_ int16_t x, int16_t y, int16_t width, int16_t height) {
  REGS;
  r.x.ax = 7;
  r.x.cx = x;
  r.x.dx = width + x;
  INT86(r);
  r.x.ax = 8;
  r.x.cx = y + 40;
  r.x.dx = y + height + 40;
  INT86(r);
}

/* 062d:0094 (MAIN 05a8:000c). INT 33h reset, then limit to 640x400; returns the reset AX. */
uint16_t opeu_mouse_reset(STATE) {
  REGS;
  r.x.ax = 0;
  INT86(r);
  opeu_mouse_window(WITH_ 0, 0, 640, 400);
  return r.x.ax;
}

/* 062d:00c6 (MAIN 05a8:0104). Queries the driver even without a mouse. */
int opeu_mouse_left(STATE) {
  REGS;
  r.x.ax = 3;
  INT86(r);
  return (r.x.bx & 1) ? -1 : 0;
}

/* 062d:00f0 (MAIN 05a8:012e). */
int opeu_mouse_right(STATE) {
  REGS;
  r.x.ax = 3;
  INT86(r);
  return (r.x.bx & 2) ? -1 : 0;
}

/* 062d:011a (MAIN 05a8:0158). */
int opeu_turn_input_a(STATE) {
  return opeu_mouse_left(WITH) || opeu_mouse_right(WITH);
}

/* 062d:0132 (MAIN 0a04:0132 with a 0x5000-byte buffer at offset 0). */
uint8_t opeu_stream_file_byte(NOARGS) {
  if (STREAM_COUNT == 0) {
    STREAM_SEG = IMAGE_SEG;
    STREAM_COUNT = READ_FAR(PICTURE_HANDLE, IMAGE_SEG, STREAM_OFF = 0, 0x5000);
  }
  STREAM_COUNT--;
  return STREAM_NEXT();
}

/* 062d:016a (MAIN 0a04:0250 on the picture file). DX:AX=position. */
void opeu_stream_seek(STATE_ int32_t position) {
  if (SEEK(PICTURE_HANDLE, position, 0) != -1) STREAM_COUNT = 0;
}

/* 062d:018e (bytes of MAIN 0a04:0272). AX=x, DX=y, stack position; RETF 4. */
void opeu_picture_file_at(STATE_ uint16_t x, uint16_t y, int32_t position) {
  opeu_stream_seek(ARGS position);
  DECODE_MASKED(x, y, FILE_READER);
}

/* 062d:01bc. BX=palette: copy it to the current palette and load the DAC. */
void opeu_open_palette_set(STATE_ PAL palette) {
  int16_t k;
  PAL src;
  PAL dst;
  src = palette;
  dst = CURRENT;
  for (k = 0; k < 48; k++, src++, dst++) *dst = *src;
  SET_PALETTE(0, 16, dst = CURRENT);
}

/* 062d:01fe. BX=target: moves every current component one step toward it,
 * loads the DAC if anything changed and reports that. */
int16_t opeu_open_palette_step(STATE_ PAL target) {
  int16_t dirty;
  uint16_t k;
  PAL src;
  PAL dst;
  dirty = 0;
  src = target;
  dst = CURRENT;
  for (k = 0; k < 48; k++, src++, dst++) {
    if (*dst != *src) {
      dirty = 1;
      if (*dst > *src)
        (*dst)--;
      else
        (*dst)++;
    }
  }
  if (dirty) SET_PALETTE(0, 16, dst = CURRENT);
  return dirty;
}

/* 062d:0262. BX=target, AX=mode: fades completely. mode -1 waits for the
 * driver timer between steps; any other mode waits a tick and returns 1 if
 * the input poll aborts the fade. */
int16_t opeu_open_palette_fade(STATE_ PAL target, uint16_t mode) {
  int16_t wait, dirty;
  uint16_t k;
  PAL src;
  PAL dst;
  wait = mode == 0xffff ? 0 : 1;
  do {
    dirty = 0;
    src = target;
    dst = CURRENT;
    for (k = 0; k < 48; k++, src++, dst++) {
      if (*dst != *src) {
        dirty = 1;
        if (*dst > *src)
          (*dst)--;
        else
          (*dst)++;
      }
    }
    if (dirty) {
      SET_PALETTE(0, 16, dst = CURRENT);
      if (wait) {
        if (opeu_open_wait_ticks(WITH_ 1)) return 1;
      } else
        DELAY(0);
    }
  } while (dirty);
  return 0;
}

/* 062d:0304. Abortable fade to black. */
int16_t opeu_open_fade_in(STATE) {
  return opeu_open_palette_fade(WITH_ BLACK, 2);
}

/* 062d:0310. Fade to black. */
int16_t opeu_open_fade_in_wait(STATE) {
  return opeu_open_palette_fade(WITH_ BLACK, 0xffff);
}

/* 062d:031c. Fade to DS:1944. */
int16_t opeu_open_fade_alt(STATE) {
  return opeu_open_palette_fade(WITH_ FADE_TARGET, 0xffff);
}

/* 062d:0328. */
void opeu_open_identity_palette(STATE) {
  IDENTITY_PALETTE();
}

/* 062d:032e. 1 (input flushed) for a key, 2 for a mouse button, else 0. */
int16_t opeu_open_input_poll(STATE) {
  if (INPUT_B()) {
    FLUSH();
    return 1;
  }
  if (opeu_turn_input_a(WITH)) return 2;
  return 0;
}

/* 062d:0350. BX=name: opens it read/write or reports it and hangs. */
int16_t opeu_open_file_or_die(STATE_ NAME name) {
  int16_t handle;
  handle = OPEN(name, 2);
  if (handle == -1) {
    CLEAR();
    ERROR_PRINT(name);
    HANG();
  }
  return handle;
}

/* 062d:0388 (MAIN 0977:0038). */
void opeu_disk_reset(uint8_t ds[65536], UtilService event) {
  LOCALS;
  DISK_RESET();
}

/* 062d:03a4. ESC '*' clears the text screen. */
void opeu_open_text_clear(STATE) {
  TEXT_PUT(0x1b);
  TEXT_PUT(0x2a);
}

/* 062d:03b4 (MAIN 0a04:065a). */
void opeu_ui_outline(STATE_ uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  LINE(x1, y1, x2, y1, color);
  LINE(x2, y1, x2, y2, color);
  LINE(x1, y2, x2, y2, color);
  LINE(x1, y1, x1, y2, color);
}

/* 062d:0416, 0418, 041a: empty routines called by the text engine (OTEXT). */
void opeu_open_stub_a(STATE) {
  UNUSED();
}

void opeu_open_stub_b(STATE) {
  UNUSED();
}

void opeu_open_stub_c(STATE) {
  UNUSED();
}
