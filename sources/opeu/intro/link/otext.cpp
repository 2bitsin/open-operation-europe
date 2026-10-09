/* Link: open/core/otext.c (OPEN 02a0:0002..07c1, key read, formatter and
 * the text console). The module takes a host structure {event, sp}: sp is
 * the modeled DGROUP stack pointer below which it places its conversion
 * buffer (SS = DS in OPEN). Events, from the host branch's comment:
 *
 *  0 keyboard read: INT 16h AH=0, AX = scan:ASCII (port_key_read)
 *  1 keyboard poll  0000:13d1 OSYSLIB = MAIN 4066 (opeu_turn_input_b)
 *  2 stub 062d:0416 (value)   OUTIL opeu_open_stub_a (an empty routine)
 *  3 box (x, y, w, h, colour) 0000:0939 OVGALIB = MAIN 2AD2 (opeu_ui_box)
 *  4 screen clear             0000:0913 OVGALIB = MAIN 2AAC (opeu_vga_clear)
 *  5 stub 062d:0418 (value)   OUTIL opeu_open_stub_b
 *  6 stub 062d:041a (value)   OUTIL opeu_open_stub_c
 *
 * The opening uses the console only to reset it (ESC R / C / M / B at
 * start-up) and for the fatal open and allocation messages; its glyph
 * routine is empty in OPEN itself.
 */
#include "port.hpp"
#include "services.hpp"
#include "process.hpp"

#define ARG(i) ((uint16_t)((i) < count ? args[(i)] : 0))

static TextHost host;

uint16_t link_otext(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: return port_key_read();
    case 1: return opeu_turn_input_b();
    case 2: opeu_open_stub_a(ds, link_outil); return 0;
    case 3: return (uint16_t)opeu_ui_box(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4));
    case 4: return (uint16_t)opeu_vga_clear();
    case 5: opeu_open_stub_b(ds, link_outil); return 0;
    case 6: opeu_open_stub_c(ds, link_outil); return 0;
    default: port_unhandled("otext", id, count);
    }
    return 0;
}

/* The caller's SP minus the return address: the text engine's free stack. */
TextHost *link_otext_host(void)
{
    host.event = link_otext;
    host.sp = (uint16_t)(open_sp - 4);
    return &host;
}

/* cursor_print(format, ...): the caller pushes the argument words; the
 * variadic host entry receives the DS offset of the first (the format). */
void link_otext_print(uint8_t *ds, const uint16_t *words, unsigned count)
{
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
