/* Link: open/core/outil.c (OPEN 062d:0008..041b, the opening's helpers:
 * mouse, picture stream, palette fades, input poll, open-or-die). Services
 * from the host branch's comment; results are DX:AX:
 *
 *  0 fm_busy()                      0000:1337 = MAIN 405F (counter B)
 *  1 int86(0x33) REGS               the mouse driver: AX 0 reset, 3 state,
 *                                   7/8 ranges (the functions OUTIL issues)
 *  2 int86(0x21) (number, AH)       AH=0Dh disk reset (port_disk_reset)
 *  3 dos_read_far(h, seg, off, n)   OFILEIO = MAIN FILEIO
 *  4 dos_seek(h, lo, hi, origin)    OFILEIO = MAIN FILEIO
 *  5 set_palette(first, count, p)   OVGALIB = MAIN 23D5, after the
 *                                   vertical-retrace wait (vga_retrace_wait)
 *  6 identity_palette()             OVGALIB = MAIN 23C4
 *  7 ui_line(x1, y1, x2, y2, c)     OVGALIB 1182 = MAIN 2FA5 opeu_ui_fill
 *                                   (the DOS build binds LINE to it)
 *  8 picture_decode_masked(x, y, reader off, reader seg)
 *                                   OVGADEC = MAIN 3490; the reader is
 *                                   062d:0132, OUTIL's opeu_stream_file_byte
 *  9 vga_clear()                    OVGALIB = MAIN 2AAC
 * 10 turn_input_b()                 OSYSLIB = MAIN 4066 (INT 16h AH=1)
 * 11 input_flush()                  02a0:0022 OTEXT
 * 12 cursor_print(format, name)     02a0:073e OTEXT
 * 13 the open-error spin            the original loops forever; here each
 *                                   turn pumps host events and continues
 *                                   (window close / headless --ms end it)
 * 14 text_put(c)                    02a0 OTEXT
 * 15 open_delay(ticks)              03ac:0004 OPENMAIN
 * 16 far byte read (seg, off)       the stream pointer DS:1c0e (data)
 * 17 dos_open(name, mode)           OFILEIO = MAIN FILEIO
 */
#include "port.hpp"
#include "services.hpp"
#include "process.hpp"
#include "open-services.hpp"
#include "hostinput.hpp"                  /* port_time_us */

static uint8_t *reader_ds;

/* The picture decoder's reader (a far call to 062d:0132 in the original). */
static uint8_t stream_reader(void)
{
    return opeu_stream_file_byte(reader_ds, link_outil);
}

/* OVGALIB 01AC (= MAIN 23D5) polls 3DAh bit 3 until the display is out of
 * vertical retrace and then until the next retrace starts, before writing
 * the DAC. The native opeu_set_palette (port/services/vgalib.c) leaves the
 * wait out; MAIN does not time anything by it, but OPEN's palette fades
 * (outil.c's fade loops: one set_palette per step, no other delay) are paced
 * by it, one step per displayed frame. Platform edit (timing): wait here
 * until the host clock passes the next frame boundary of mode 12h's
 * 59.940476 Hz refresh (the 25.175 MHz dot clock / (800 x 525)). The
 * boundaries are absolute multiples of the period, so the pump's 1 ms
 * granularity never accumulates drift. */
static void vga_retrace_wait(void)
{
    const uint64_t num = (uint64_t)800 * 525 * 1000000, den = 25175000;  /* period in us = num / den */
    uint64_t k = port_time_us() * den / num + 1;      /* the next frame */
    uint64_t next = (k * num + den - 1) / den;
    while (port_time_us() < next)
        port_pump();
}

static void mouse_int33(uint16_t *w)
{
    switch (w[0]) {
    case 0:
        w[0] = port_mouse_reset();
        w[1] = 2;                       /* BX: number of buttons */
        break;
    case 3: port_mouse_state(&w[1], &w[2], &w[3]); break;
    case 7: port_mouse_x_range(w[2], w[3]); break;
    case 8: port_mouse_y_range(w[2], w[3]); break;
    default: port_fatal("OPEN: INT 33h function %04x is not issued by the opening", w[0]);
    }
    w[6] = 0;                           /* cflag */
}

uint32_t link_outil(uint8_t *ds, uint16_t id, uint16_t *words, uint16_t count)
{
#define W(i) ((uint16_t)((i) < count ? words[(i)] : 0))
    switch (id) {
    case 0: return opeu_fm_busy();
    case 1: mouse_int33(words); return words[0];
    case 2:
        if (W(0) != 0x21 || W(1) != 0x0d)
            port_fatal("OPEN: int86(%02x) AH=%02x is not issued by the opening", W(0), W(1));
        port_disk_reset();
        return 0;
    case 3: return (uint16_t)opeu_dos_read_far((int16_t)W(0), W(1), W(2), W(3));
    case 4: return (uint32_t)opeu_dos_seek((int16_t)W(0), (int32_t)((uint32_t)W(1) | (uint32_t)W(2) << 16), (int16_t)W(3));
    case 5:
        vga_retrace_wait();
        return (uint16_t)opeu_set_palette((uint8_t)W(0), (uint8_t)W(1), ds + W(2));
    case 6: koei_vga_identity_palette(); return 0;
    case 7: opeu_ui_fill(W(0), W(1), W(2), W(3), W(4)); return 0;
    case 8: {
        uint8_t *saved = reader_ds;
        int16_t r;
        if (W(2) != 0x0132 || W(3) != 0x062d)
            port_fatal("OPEN: picture reader %04x:%04x is not OUTIL's stream reader", W(3), W(2));
        reader_ds = ds;
        r = opeu_picture_decode_masked(W(0), W(1), stream_reader);
        reader_ds = saved;
        return (uint16_t)r;
    }
    case 9: return (uint16_t)opeu_vga_clear();
    case 10: return opeu_turn_input_b();
    case 11: opeu_input_flush(ds, link_otext_host()); return 0;
    case 12: link_otext_print(ds, words, 2); return 0;
    case 13: port_pump(); return 0;
    case 14: return opeu_text_put(ds, link_otext_host(), (uint8_t)W(0));
    case 15: opeu_open_delay(ds, link_openmain, W(0)); return 0;
    case 16: return *port_far(W(0), W(1));
    case 17: return (uint16_t)opeu_dos_open((const char *)ds + W(0), W(1));
    default: port_unhandled("outil", id, count);
    }
    return 0;
#undef W
}
