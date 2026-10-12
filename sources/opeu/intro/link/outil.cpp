/* Link: recovered/core/outil.cpp (OPEN 062d:0008..041b, the opening's
 * helpers: mouse, picture stream, palette fades, input poll, open-or-die).
 * Results are DX:AX. The library services and their MAIN originals: FM_BUSY
 * and TURN_INPUT_B are OSYSLIB = MAIN 405F and 4066; DOS_* are OFILEIO = MAIN
 * FILEIO (DOS_INT21 is only the disk reset, AH=0Dh); SET_PALETTE, IDENTITY_
 * PALETTE and VGA_CLEAR are OVGALIB = MAIN 23D5, 23C4 and 2AAC; UI_LINE is
 * OVGALIB 1182 = MAIN 2FA5 opeu_ui_fill; PICTURE_DECODE_MASKED is OVGADEC =
 * MAIN 3490 with OUTIL's stream_file_byte as its reader. The open-error spin
 * pumps host events on each turn and continues (window close / headless --ms
 * end it); MOUSE_INT33 serves the functions OUTIL issues (AX 0 reset, 3
 * state, 7/8 ranges).
 */
#include "hostinput.hpp" /* port_time_us */
#include "open-services.hpp"
#include "port.hpp"
#include "process.hpp"
#include "services.hpp"

namespace opeu::intro {
namespace {
uint8_t* reader_ds;

/* The picture decoder's reader (a far call to 062d:0132 in the original). */
auto stream_reader() -> uint8_t {
  return opeu_stream_file_byte(reader_ds, link_outil);
}

/* OVGALIB 01AC (= MAIN 23D5) polls 3DAh bit 3 until the display is out of
 * vertical retrace and then until the next retrace starts, before writing
 * the DAC. The native opeu_set_palette (services/vgalib.cpp) leaves the
 * wait out; MAIN does not time anything by it, but OPEN's palette fades
 * (outil.cpp's fade loops: one set_palette per step, no other delay) are paced
 * by it, one step per displayed frame. Platform edit (timing): wait here
 * until the host clock passes the next frame boundary of mode 12h's
 * 59.940476 Hz refresh (the 25.175 MHz dot clock / (800 x 525)). The
 * boundaries are absolute multiples of the period, so the pump's 1 ms
 * granularity never accumulates drift. */
void vga_retrace_wait() {
  const uint64_t num  = static_cast<uint64_t>(800) * 525 * 1000000;
  const uint64_t den  = 25175000;                         /* period in us = num / den */
  uint64_t       k    = (port_time_us() * den / num) + 1; /* the next frame */
  uint64_t       next = ((k * num) + den - 1) / den;
  while (port_time_us() < next) port_pump();
}

void mouse_int33(uint16_t* w) {
  switch (w[0]) {
  case 0:
    w[0] = port_mouse_reset();
    w[1] = 2; /* BX: number of buttons */
    break;
  case 3:  port_mouse_state(&w[1], &w[2], &w[3]); break;
  case 7:  port_mouse_x_range(w[2], w[3]); break;
  case 8:  port_mouse_y_range(w[2], w[3]); break;
  default: port_fatal("OPEN: INT 33h function %04x is not issued by the opening", w[0]);
  }
  w[6] = 0; /* cflag */
}
}

auto link_outil(uint8_t* ds, UtilServiceId id, uint16_t* words, uint16_t count) -> uint32_t {
  using enum UtilServiceId;
  auto const arg{ [&](int i) -> uint16_t { return Arg(words, count, i); } };
  switch (id) {
  case FM_BUSY:     return opeu_fm_busy();
  case MOUSE_INT33: mouse_int33(words); return words[0];
  case DOS_INT21:
    if (arg(0) != 0x21 || arg(1) != 0x0d) port_fatal("OPEN: int86(%02x) AH=%02x is not issued by the opening", arg(0), arg(1));
    port_disk_reset();
    return 0;
  case DOS_READ_FAR:          return static_cast<uint16_t>(opeu_dos_read_far(static_cast<int16_t>(arg(0)), arg(1), arg(2), arg(3)));
  case DOS_SEEK:              return static_cast<uint32_t>(opeu_dos_seek(static_cast<int16_t>(arg(0)), static_cast<int32_t>(Arg32(words, count, 1)), static_cast<int16_t>(arg(3))));
  case SET_PALETTE:           vga_retrace_wait(); return static_cast<uint16_t>(opeu_set_palette(static_cast<uint8_t>(arg(0)), static_cast<uint8_t>(arg(1)), ds + arg(2)));
  case IDENTITY_PALETTE:      koei_vga_identity_palette(); return 0;
  case UI_LINE:               opeu_ui_fill(arg(0), arg(1), arg(2), arg(3), arg(4)); return 0;
  case PICTURE_DECODE_MASKED: {
    uint8_t* saved = reader_ds;
    int16_t  r     = 0;
    if (arg(2) != STREAM_READER_OFFSET || arg(3) != STREAM_READER_SEGMENT) port_fatal("OPEN: picture reader %04x:%04x is not OUTIL's stream reader", arg(3), arg(2));
    reader_ds = ds;
    r         = opeu_picture_decode_masked(arg(0), arg(1), stream_reader);
    reader_ds = saved;
    return static_cast<uint16_t>(r);
  }
  case VGA_CLEAR:       return static_cast<uint16_t>(opeu_vga_clear());
  case TURN_INPUT_B:    return opeu_turn_input_b();
  case INPUT_FLUSH:     opeu_input_flush(ds, link_otext_host()); return 0;
  case CURSOR_PRINT:    link_otext_print(ds, words, 2); return 0;
  case OPEN_ERROR_SPIN: port_pump(); return 0;
  case TEXT_PUT:        return opeu_text_put(ds, link_otext_host(), static_cast<uint8_t>(arg(0)));
  case OPEN_DELAY:      opeu_open_delay(ds, link_openmain, arg(0)); return 0;
  case FAR_BYTE:        return *port_far(arg(0), arg(1));
  case DOS_OPEN:        return static_cast<uint16_t>(opeu_dos_open(reinterpret_cast<const char*>(ds) + arg(0), arg(1)));
  default:              port_unhandled("outil", static_cast<unsigned>(id), count);
  }
  return 0;
}

}
