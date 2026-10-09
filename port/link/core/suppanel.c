/* Link: core/suppanel.c (20b3:0116..0451 supply-allocation panel). Services,
 * from the host branch:
 *  1 mouse show        05a8:003e     2 delay ticks(t)    0a8e:000c
 *  3 mouse position: no arguments; the host reads x, y back from args[0],
 *    args[1] (DOS: opeu_mouse_position(&x, &y), 05a8:0086)
 *  4 right down -> AX  05a8:012e     5 left down -> AX   05a8:0104
 *  6 mouse hide        05a8:0062     7 keyboard key -> AL 0541:00ba
 *  8 restore(x,y,w,h,mode,segment,offset)  VGALIB 2674 opeu_bitmap_restore
 *  9 army_position(x,y) 0480:0808   10 army_print(format,value) 0480:0bb8
 * 11 glyph(x,y,w,h,colour,near glyph)  VGALIB 2A81 opeu_vga_glyph */
#include "b2_common.h"

uint16_t link_suppanel(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 1: opeu_mouse_show(ds, link_mouse); break;
    case 2: opeu_delay_ticks(ds, link_waitkey, ARG(0)); break;
    case 3: {
        /* x/y words in the modeled stack, copied back to the caller's words. */
        uint16_t at = (uint16_t)(port_stack_enter(4) - 4);
        uint16_t *out = (uint16_t *)(uintptr_t)args;
        opeu_mouse_position(ds, link_mouse, at, (uint16_t)(at + 2));
        out[0] = port_ds_word(at);
        out[1] = port_ds_word((uint16_t)(at + 2));
        port_stack_leave(4);
        break;
    }
    case 4: r = (uint16_t)opeu_mouse_right(ds, link_mouse); break;
    case 5: r = (uint16_t)opeu_mouse_left(ds, link_mouse); break;
    case 6: opeu_mouse_hide(ds, link_mouse); break;
    case 7: r = opeu_keyboard_key(ds, link_keyboard); break;
    case 8: opeu_bitmap_restore(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), B2_FAR(ARG(5), ARG(6))); break;
    case 9: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 10: b2_cursor_print(ds, args, count); break;
    case 11: opeu_vga_glyph(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), PORT_DS(ARG(5))); break;
    default: port_unhandled("suppanel", id, count);
    }
    return r;
}
