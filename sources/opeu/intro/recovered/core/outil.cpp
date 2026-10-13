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

auto util_call(Open& open, UtilService event, UtilServiceId id, uint16_t count, uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e) -> uint32_t {
  uint16_t words[5];
  words[0] = a;
  words[1] = b;
  words[2] = c;
  words[3] = d;
  words[4] = e;
  return event(open, id, words, count);
}

/* One service call with its argument words. */
template <class... Words>
auto Call(Open& open, UtilService event, UtilServiceId id, Words... words) -> uint32_t {
  uint16_t const padded[5]{ static_cast<uint16_t>(words)... };
  return util_call(open, event, id, sizeof...(Words), padded[0], padded[1], padded[2], padded[3], padded[4]);
}

auto util_int86(Open& open, UtilService event, Regs* r) -> void {
  uint16_t words[7];
  words[0] = r->x.ax;
  words[1] = r->x.bx;
  words[2] = r->x.cx;
  words[3] = r->x.dx;
  words[4] = r->x.si;
  words[5] = r->x.di;
  words[6] = r->x.cflag;
  event(open, MOUSE_INT33, words, 7);
  r->x.ax    = words[0];
  r->x.bx    = words[1];
  r->x.cx    = words[2];
  r->x.dx    = words[3];
  r->x.si    = words[4];
  r->x.di    = words[5];
  r->x.cflag = words[6];
}

/* *stream++ through the far pointer DS:1c0e (offset), 1c10 (segment). */
auto util_stream_next(Open& open, UtilService event) -> uint8_t {
  uint16_t offset          = Data(open).stream_offset;
  Data(open).stream_offset = static_cast<uint16_t>(offset + 1);
  return static_cast<uint8_t>(Call(open, event, FAR_BYTE, Data(open).stream_segment, offset));
}
}

/* 062d:0008. AX=ticks. Waits until the FM driver's down-counter has run
 * `ticks` further; 1 if the input poll reported a key or button first. */
auto opeu_open_wait_ticks(Open& open, UtilService event, uint16_t ticks) -> int16_t {
  uint16_t target  = 0;
  target           = static_cast<uint16_t>(Call(open, event, FM_BUSY));
  target          -= ticks;
  do {
    if (opeu_open_input_poll(open, event) != 0) return 1;
  } while (static_cast<uint16_t>(Call(open, event, FM_BUSY)) > target);
  return 0;
}

/* 062d:003e (MAIN 05a8:0210). AX=x, DX=y, BX=width, stack height; RETF 2. */
void opeu_mouse_window(Open& open, UtilService event, int16_t x, int16_t y, int16_t width, int16_t height) {
  Regs r = {};
  r.x.ax = 7;
  r.x.cx = static_cast<uint16_t>(x);
  r.x.dx = static_cast<uint16_t>(width + x);
  util_int86(open, event, &r);
  r.x.ax = 8;
  r.x.cx = static_cast<uint16_t>(y + 40);
  r.x.dx = static_cast<uint16_t>(y + height + 40);
  util_int86(open, event, &r);
}

/* 062d:0094 (MAIN 05a8:000c). INT 33h reset, then limit to 640x400; returns the reset AX. */
auto opeu_mouse_reset(Open& open, UtilService event) -> uint16_t {
  Regs r = {};
  r.x.ax = 0;
  util_int86(open, event, &r);
  opeu_mouse_window(open, event, 0, 0, 640, 400);
  return r.x.ax;
}

/* 062d:00c6 (MAIN 05a8:0104). Queries the driver even without a mouse. */
auto opeu_mouse_left(Open& open, UtilService event) -> int {
  Regs r = {};
  r.x.ax = 3;
  util_int86(open, event, &r);
  return ((r.x.bx & 1) != 0) ? -1 : 0;
}

/* 062d:00f0 (MAIN 05a8:012e). */
auto opeu_mouse_right(Open& open, UtilService event) -> int {
  Regs r = {};
  r.x.ax = 3;
  util_int86(open, event, &r);
  return ((r.x.bx & 2) != 0) ? -1 : 0;
}

/* 062d:011a (MAIN 05a8:0158). */
auto opeu_turn_input_a(Open& open, UtilService event) -> int {
  return static_cast<int>((static_cast<int>(opeu_mouse_left(open, event) != 0) != 0) || (opeu_mouse_right(open, event) != 0));
}

/* 062d:0132 (MAIN 0a04:0132 with a 0x5000-byte buffer at offset 0). */
auto opeu_stream_file_byte(Open& open, UtilService event) -> uint8_t {
  if (Data(open).stream_count == 0) {
    Data(open).stream_segment = Data(open).buffer_g;
    Data(open).stream_count   = static_cast<uint16_t>(Call(open, event, DOS_READ_FAR, Data(open).picture_handle, Data(open).buffer_g, Data(open).stream_offset = 0, 0x5000));
  }
  Data(open).stream_count--;
  return util_stream_next(open, event);
}

/* 062d:016a (MAIN 0a04:0250 on the picture file). DX:AX=position. */
void opeu_stream_seek(Open& open, UtilService event, int32_t position) {
  if (static_cast<int32_t>(Call(open, event, DOS_SEEK, Data(open).picture_handle, position, static_cast<uint32_t>(position) >> 16, 0)) != -1) Data(open).stream_count = 0;
}

/* 062d:018e (bytes of MAIN 0a04:0272). AX=x, DX=y, stack position; RETF 4. */
void opeu_picture_file_at(Open& open, UtilService event, uint16_t x, uint16_t y, int32_t position) {
  opeu_stream_seek(open, event, position);
  Call(open, event, PICTURE_DECODE_MASKED, x, y, STREAM_READER_OFFSET, STREAM_READER_SEGMENT);
}

/* 062d:01bc. BX=palette: copy it to the current palette and load the DAC. */
void opeu_open_palette_set(Open& open, UtilService event, uint8_t* palette) {
  int16_t  k   = 0;
  uint8_t* src = nullptr;
  uint8_t* dst = nullptr;
  src          = palette;
  dst          = Data(open).palette;
  for (k = 0; k < 48; k++, src++, dst++) *dst = *src;
  Call(open, event, SET_PALETTE, 0, 16, Near(open, Data(open).palette));
}

/* 062d:01fe. BX=target: moves every current component one step toward it,
 * loads the DAC if anything changed and reports that. */
auto opeu_open_palette_step(Open& open, UtilService event, uint8_t* target) -> int16_t {
  int16_t  dirty = 0;
  uint16_t k     = 0;
  uint8_t* src   = nullptr;
  uint8_t* dst   = nullptr;
  dirty          = 0;
  src            = target;
  dst            = Data(open).palette;
  for (k = 0; k < 48; k++, src++, dst++) {
    if (*dst != *src) {
      dirty = 1;
      if (*dst > *src)
        (*dst)--;
      else
        (*dst)++;
    }
  }
  if (dirty != 0) Call(open, event, SET_PALETTE, 0, 16, Near(open, Data(open).palette));
  return dirty;
}

/* 062d:0262. BX=target, AX=mode: fades completely. mode -1 waits for the
 * driver timer between steps; any other mode waits a tick and returns 1 if
 * the input poll aborts the fade. */
auto opeu_open_palette_fade(Open& open, UtilService event, uint8_t* target, uint16_t mode) -> int16_t {
  int16_t  wait  = 0;
  int16_t  dirty = 0;
  uint16_t k     = 0;
  uint8_t* src   = nullptr;
  uint8_t* dst   = nullptr;
  wait           = mode == 0xffff ? 0 : 1;
  do {
    dirty = 0;
    src   = target;
    dst   = Data(open).palette;
    for (k = 0; k < 48; k++, src++, dst++) {
      if (*dst != *src) {
        dirty = 1;
        if (*dst > *src)
          (*dst)--;
        else
          (*dst)++;
      }
    }
    if (dirty != 0) {
      Call(open, event, SET_PALETTE, 0, 16, Near(open, Data(open).palette));
      if (wait != 0) {
        if (opeu_open_wait_ticks(open, event, 1) != 0) return 1;
      } else {
        Call(open, event, OPEN_DELAY, 0);
      }
    }
  } while (dirty != 0);
  return 0;
}

/* 062d:0304. Abortable fade to black. */
auto opeu_open_fade_in(Open& open, UtilService event) -> int16_t {
  return opeu_open_palette_fade(open, event, Data(open).black, 2);
}

/* 062d:0310. Fade to black. */
auto opeu_open_fade_in_wait(Open& open, UtilService event) -> int16_t {
  return opeu_open_palette_fade(open, event, Data(open).black, 0xffff);
}

/* 062d:031c. Fade to DS:1944. */
auto opeu_open_fade_alt(Open& open, UtilService event) -> int16_t {
  return opeu_open_palette_fade(open, event, Data(open).fade_target, 0xffff);
}

/* 062d:0328. */
void opeu_open_identity_palette(Open& open, UtilService event) {
  Call(open, event, IDENTITY_PALETTE);
}

/* 062d:032e. 1 (input flushed) for a key, 2 for a mouse button, else 0. */
auto opeu_open_input_poll(Open& open, UtilService event) -> int16_t {
  if ((static_cast<int16_t>(Call(open, event, TURN_INPUT_B))) != 0) {
    Call(open, event, INPUT_FLUSH);
    return 1;
  }
  if (opeu_turn_input_a(open, event) != 0) return 2;
  return 0;
}

/* 062d:0350. BX=name: opens it read/write or reports it and hangs. */
auto opeu_open_file_or_die(Open& open, UtilService event, uint16_t name) -> int16_t {
  int16_t handle = 0;
  handle         = static_cast<int16_t>(Call(open, event, DOS_OPEN, name, 2));
  if (handle == -1) {
    Call(open, event, VGA_CLEAR);
    Call(open, event, CURSOR_PRINT, Near(open, Data(open).error_format), name);
    while (Call(open, event, OPEN_ERROR_SPIN) == 0u);
  }
  return handle;
}

/* 062d:0388 (MAIN 0977:0038). */
void opeu_disk_reset(Open& open, UtilService event) {
  Call(open, event, DOS_INT21, 0x21, 0x0d);
}

/* 062d:03a4. ESC '*' clears the text screen. */
void opeu_open_text_clear(Open& open, UtilService event) {
  Call(open, event, TEXT_PUT, 0x1b);
  Call(open, event, TEXT_PUT, 0x2a);
}

/* 062d:03b4 (MAIN 0a04:065a). */
void opeu_ui_outline(Open& open, UtilService event, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  Call(open, event, UI_LINE, x1, y1, x2, y1, color);
  Call(open, event, UI_LINE, x2, y1, x2, y2, color);
  Call(open, event, UI_LINE, x1, y2, x2, y2, color);
  Call(open, event, UI_LINE, x1, y1, x1, y2, color);
}

/* 062d:0416, 0418, 041a: empty routines called by the text engine (OTEXT). */
void opeu_open_stub_a([[maybe_unused]] Open& open, [[maybe_unused]] UtilService event) { }

void opeu_open_stub_b([[maybe_unused]] Open& open, [[maybe_unused]] UtilService event) { }

void opeu_open_stub_c([[maybe_unused]] Open& open, [[maybe_unused]] UtilService event) { }

}
