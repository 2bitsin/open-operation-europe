/* OPEN 062d:0008..041b (image 0x62d8..0x66ec): the opening's helper
 * segment. Its routines come from several MAIN files; the ones that are
 * byte-identical in OPEN keep MAIN's C text: the mouse services (MOUSE 05a8),
 * the picture/outline helpers (UIPRIM 0a04) and the disk reset (RESOPEN
 * 0977). The rest is OPEN's: a tick wait with input abort, the image-stream
 * variants that always read the picture file DS:0440, the palette
 * set/step/fade helpers on the current palette DS:0358, the fade wrappers,
 * the input poll, the open-or-die file opener, the text clear and three empty
 * routines the text engine calls.
 *
 * ds is DGROUP; near pointers are DS offsets except the palette pointers,
 * which are host pointers into ds. Every routine outside this segment is a
 * service (UtilServiceId) returning DX:AX. The original spins forever after
 * an open error; here each turn asks the service, so a caller can end the
 * spin.
 */
#include "recovered.hpp"

namespace opeu::intro::detail::outil {
namespace {
using enum UtilServiceId;

auto util_call(uint8_t* ds, UtilService event, UtilServiceId id, uint16_t count, uint16_t a, uint16_t b, uint16_t c,
               uint16_t d, uint16_t e) -> uint32_t {
  uint16_t words[5];
  words[0] = a;
  words[1] = b;
  words[2] = c;
  words[3] = d;
  words[4] = e;
  return event(ds, id, words, count);
}

/* One service call with its argument words. */
template <class... Words>
auto Call(uint8_t* ds, UtilService event, UtilServiceId id, Words... words) -> uint32_t {
  uint16_t const padded[5]{ static_cast<uint16_t>(words)... };
  return util_call(ds, event, id, sizeof...(Words), padded[0], padded[1], padded[2], padded[3], padded[4]);
}

auto util_int86(uint8_t* ds, UtilService event, Regs* r) -> void {
  uint16_t words[7];
  words[0] = r->x.ax;
  words[1] = r->x.bx;
  words[2] = r->x.cx;
  words[3] = r->x.dx;
  words[4] = r->x.si;
  words[5] = r->x.di;
  words[6] = r->x.cflag;
  event(ds, MOUSE_INT33, words, 7);
  r->x.ax = words[0];
  r->x.bx = words[1];
  r->x.cx = words[2];
  r->x.dx = words[3];
  r->x.si = words[4];
  r->x.di = words[5];
  r->x.cflag = words[6];
}

/* *stream++ through the far pointer DS:1c0e (offset), 1c10 (segment). */
auto util_stream_next(uint8_t* ds, UtilService event) -> uint8_t {
  uint16_t offset = Data(ds).stream_offset;
  Data(ds).stream_offset = (uint16_t)(offset + 1);
  return (uint8_t)Call(ds, event, FAR_BYTE, Data(ds).stream_segment, offset);
}
}

/* 062d:0008. AX=ticks. Waits until the FM driver's down-counter has run
 * `ticks` further; 1 if the input poll reported a key or button first. */
int16_t opeu_open_wait_ticks(uint8_t* ds, UtilService event, uint16_t ticks) {
  uint16_t target;
  target = (uint16_t)Call(ds, event, FM_BUSY);
  target -= ticks;
  do {
    if (opeu_open_input_poll(ds, event)) return 1;
  } while ((uint16_t)Call(ds, event, FM_BUSY) > target);
  return 0;
}

/* 062d:003e (MAIN 05a8:0210). AX=x, DX=y, BX=width, stack height; RETF 2. */
void opeu_mouse_window(uint8_t* ds, UtilService event, int16_t x, int16_t y, int16_t width, int16_t height) {
  Regs r = { { 0, 0, 0, 0, 0, 0, 0 } };
  r.x.ax = 7;
  r.x.cx = x;
  r.x.dx = width + x;
  util_int86(ds, event, &r);
  r.x.ax = 8;
  r.x.cx = y + 40;
  r.x.dx = y + height + 40;
  util_int86(ds, event, &r);
}

/* 062d:0094 (MAIN 05a8:000c). INT 33h reset, then limit to 640x400; returns the reset AX. */
uint16_t opeu_mouse_reset(uint8_t* ds, UtilService event) {
  Regs r = { { 0, 0, 0, 0, 0, 0, 0 } };
  r.x.ax = 0;
  util_int86(ds, event, &r);
  opeu_mouse_window(ds, event, 0, 0, 640, 400);
  return r.x.ax;
}

/* 062d:00c6 (MAIN 05a8:0104). Queries the driver even without a mouse. */
int opeu_mouse_left(uint8_t* ds, UtilService event) {
  Regs r = { { 0, 0, 0, 0, 0, 0, 0 } };
  r.x.ax = 3;
  util_int86(ds, event, &r);
  return (r.x.bx & 1) ? -1 : 0;
}

/* 062d:00f0 (MAIN 05a8:012e). */
int opeu_mouse_right(uint8_t* ds, UtilService event) {
  Regs r = { { 0, 0, 0, 0, 0, 0, 0 } };
  r.x.ax = 3;
  util_int86(ds, event, &r);
  return (r.x.bx & 2) ? -1 : 0;
}

/* 062d:011a (MAIN 05a8:0158). */
int opeu_turn_input_a(uint8_t* ds, UtilService event) {
  return opeu_mouse_left(ds, event) || opeu_mouse_right(ds, event);
}

/* 062d:0132 (MAIN 0a04:0132 with a 0x5000-byte buffer at offset 0). */
uint8_t opeu_stream_file_byte(uint8_t* ds, UtilService event) {
  if (Data(ds).stream_count == 0) {
    Data(ds).stream_segment = Data(ds).buffer_g;
    Data(ds).stream_count = (uint16_t)Call(ds, event, DOS_READ_FAR, Data(ds).picture_handle, Data(ds).buffer_g,
                                           Data(ds).stream_offset = 0, 0x5000);
  }
  Data(ds).stream_count--;
  return util_stream_next(ds, event);
}

/* 062d:016a (MAIN 0a04:0250 on the picture file). DX:AX=position. */
void opeu_stream_seek(uint8_t* ds, UtilService event, int32_t position) {
  if ((int32_t)Call(ds, event, DOS_SEEK, Data(ds).picture_handle, position, (uint32_t)position >> 16, 0) != -1)
    Data(ds).stream_count = 0;
}

/* 062d:018e (bytes of MAIN 0a04:0272). AX=x, DX=y, stack position; RETF 4. */
void opeu_picture_file_at(uint8_t* ds, UtilService event, uint16_t x, uint16_t y, int32_t position) {
  opeu_stream_seek(ds, event, position);
  Call(ds, event, PICTURE_DECODE_MASKED, x, y, STREAM_READER_OFFSET, STREAM_READER_SEGMENT);
}

/* 062d:01bc. BX=palette: copy it to the current palette and load the DAC. */
void opeu_open_palette_set(uint8_t* ds, UtilService event, uint8_t* palette) {
  int16_t k;
  uint8_t* src;
  uint8_t* dst;
  src = palette;
  dst = Data(ds).palette;
  for (k = 0; k < 48; k++, src++, dst++) *dst = *src;
  Call(ds, event, SET_PALETTE, 0, 16, (dst = Data(ds).palette) - ds);
}

/* 062d:01fe. BX=target: moves every current component one step toward it,
 * loads the DAC if anything changed and reports that. */
int16_t opeu_open_palette_step(uint8_t* ds, UtilService event, uint8_t* target) {
  int16_t dirty;
  uint16_t k;
  uint8_t* src;
  uint8_t* dst;
  dirty = 0;
  src = target;
  dst = Data(ds).palette;
  for (k = 0; k < 48; k++, src++, dst++) {
    if (*dst != *src) {
      dirty = 1;
      if (*dst > *src)
        (*dst)--;
      else
        (*dst)++;
    }
  }
  if (dirty) Call(ds, event, SET_PALETTE, 0, 16, (dst = Data(ds).palette) - ds);
  return dirty;
}

/* 062d:0262. BX=target, AX=mode: fades completely. mode -1 waits for the
 * driver timer between steps; any other mode waits a tick and returns 1 if
 * the input poll aborts the fade. */
int16_t opeu_open_palette_fade(uint8_t* ds, UtilService event, uint8_t* target, uint16_t mode) {
  int16_t wait, dirty;
  uint16_t k;
  uint8_t* src;
  uint8_t* dst;
  wait = mode == 0xffff ? 0 : 1;
  do {
    dirty = 0;
    src = target;
    dst = Data(ds).palette;
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
      Call(ds, event, SET_PALETTE, 0, 16, (dst = Data(ds).palette) - ds);
      if (wait) {
        if (opeu_open_wait_ticks(ds, event, 1)) return 1;
      } else
        Call(ds, event, OPEN_DELAY, 0);
    }
  } while (dirty);
  return 0;
}

/* 062d:0304. Abortable fade to black. */
int16_t opeu_open_fade_in(uint8_t* ds, UtilService event) {
  return opeu_open_palette_fade(ds, event, Data(ds).black, 2);
}

/* 062d:0310. Fade to black. */
int16_t opeu_open_fade_in_wait(uint8_t* ds, UtilService event) {
  return opeu_open_palette_fade(ds, event, Data(ds).black, 0xffff);
}

/* 062d:031c. Fade to DS:1944. */
int16_t opeu_open_fade_alt(uint8_t* ds, UtilService event) {
  return opeu_open_palette_fade(ds, event, Data(ds).fade_target, 0xffff);
}

/* 062d:0328. */
void opeu_open_identity_palette(uint8_t* ds, UtilService event) {
  Call(ds, event, IDENTITY_PALETTE);
}

/* 062d:032e. 1 (input flushed) for a key, 2 for a mouse button, else 0. */
int16_t opeu_open_input_poll(uint8_t* ds, UtilService event) {
  if ((int16_t)Call(ds, event, TURN_INPUT_B)) {
    Call(ds, event, INPUT_FLUSH);
    return 1;
  }
  if (opeu_turn_input_a(ds, event)) return 2;
  return 0;
}

/* 062d:0350. BX=name: opens it read/write or reports it and hangs. */
int16_t opeu_open_file_or_die(uint8_t* ds, UtilService event, uint16_t name) {
  int16_t handle;
  handle = (int16_t)Call(ds, event, DOS_OPEN, name, 2);
  if (handle == -1) {
    Call(ds, event, VGA_CLEAR);
    Call(ds, event, CURSOR_PRINT, Near(ds, Data(ds).error_format), name);
    while (!Call(ds, event, OPEN_ERROR_SPIN));
  }
  return handle;
}

/* 062d:0388 (MAIN 0977:0038). */
void opeu_disk_reset(uint8_t* ds, UtilService event) {
  Call(ds, event, DOS_INT21, 0x21, 0x0d);
}

/* 062d:03a4. ESC '*' clears the text screen. */
void opeu_open_text_clear(uint8_t* ds, UtilService event) {
  Call(ds, event, TEXT_PUT, 0x1b);
  Call(ds, event, TEXT_PUT, 0x2a);
}

/* 062d:03b4 (MAIN 0a04:065a). */
void opeu_ui_outline(uint8_t* ds, UtilService event, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                     uint16_t color) {
  Call(ds, event, UI_LINE, x1, y1, x2, y1, color);
  Call(ds, event, UI_LINE, x2, y1, x2, y2, color);
  Call(ds, event, UI_LINE, x1, y2, x2, y2, color);
  Call(ds, event, UI_LINE, x1, y1, x1, y2, color);
}

/* 062d:0416, 0418, 041a: empty routines called by the text engine (OTEXT). */
void opeu_open_stub_a([[maybe_unused]] uint8_t* ds, [[maybe_unused]] UtilService event) { }

void opeu_open_stub_b([[maybe_unused]] uint8_t* ds, [[maybe_unused]] UtilService event) { }

void opeu_open_stub_c([[maybe_unused]] uint8_t* ds, [[maybe_unused]] UtilService event) { }

}
