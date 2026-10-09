/* Link: recovered/core/otext.cpp (OPEN 02a0:0002..07c1, key read, formatter
 * and the text console). The module takes a host structure {event, sp}: sp
 * is the modeled DGROUP stack pointer below which it places its conversion
 * buffer (SS = DS in OPEN). KEY_WAITING is OSYSLIB 13d1 = MAIN 4066
 * (opeu_turn_input_b), BOX and CLEAR are OVGALIB = MAIN 2AD2 and 2AAC, and
 * the three stubs are OUTIL's empty routines.
 *
 * The opening uses the console only to reset it (ESC R / C / M / B at
 * start-up) and for the fatal open and allocation messages; its glyph
 * routine is empty in OPEN itself.
 */
#include "port.hpp"
#include "process.hpp"
#include "services.hpp"

namespace opeu::intro {
namespace {
TextHost host;
}

uint16_t link_otext(uint8_t* ds, TextEventId id, uint16_t const* args, uint16_t count) {
  using enum TextEventId;
  auto const arg{ [&](int i) { return Arg(args, count, i); } };
  switch (id) {
  case KEY_READ: return port_key_read();
  case KEY_WAITING: return opeu_turn_input_b();
  case STUB_A: opeu_open_stub_a(ds, link_outil); return 0;
  case BOX: return (uint16_t)opeu_ui_box(arg(0), arg(1), arg(2), arg(3), arg(4));
  case CLEAR: return (uint16_t)opeu_vga_clear();
  case STUB_B: opeu_open_stub_b(ds, link_outil); return 0;
  case STUB_C: opeu_open_stub_c(ds, link_outil); return 0;
  default: port_unhandled("otext", static_cast<unsigned>(id), count);
  }
  return 0;
}

/* The caller's SP minus the return address: the text engine's free stack. */
TextHost* link_otext_host(void) {
  host.event = link_otext;
  host.sp = (uint16_t)(open_sp - 4);
  return &host;
}

/* cursor_print(format, ...): the caller pushes the argument words; the
 * variadic host entry receives the DS offset of the first (the format). */
void link_otext_print(uint8_t* ds, const uint16_t* words, unsigned count) {
  uint16_t saved = open_sp, frame;
  unsigned i;
  open_sp = (uint16_t)(open_sp - 2 * count);
  frame = open_sp;
  for (i = 0; i < count; i++) {
    ds[(uint16_t)(frame + 2 * i)] = (uint8_t)words[i];
    ds[(uint16_t)(frame + 2 * i + 1)] = (uint8_t)(words[i] >> 8);
  }
  opeu_cursor_print(ds, link_otext_host(), frame);
  open_sp = saved;
}

}
