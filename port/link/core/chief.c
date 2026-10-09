/* Link: core/chief.c (20b3:000c..0115 operations-chief panel and prompts).
 * Services, from the host branch:
 *  0 fill(x,y,w,h,color)        VGALIB 2AD2 opeu_vga_fill_rect = ui_box
 *  1 frame(x,y,w)               0a04:02a0 context_frame
 *  2 format(buffer,format,l,h)  0480:0b9a request_format (cdecl)
 *  3 print(format,person)       0480:0bb8 army_print (cdecl)
 *  4 portrait(x,y,person)       0a04:046c context_portrait
 *  5 position(x,y)              0480:0808 army_position
 *  6 message(text)              1707:0652 message_text = text_feed
 *  7 wait(text) -> AX           1707:075a message_wait = confirm_prompt
 *  8 input(low,high,value,d)->AX 0541:0238 number_input = choose_value
 *    (min, max, cancel, items: the original passes the four words as is). */
#include "b2_common.h"

uint16_t link_chief(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_ui_box(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 1: opeu_context_frame(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 2: b2_request_format(ds, args, count); break;
    case 3: b2_cursor_print(ds, args, count); break;
    case 4: opeu_context_portrait(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 5: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 6: opeu_text_feed(ds, link_dialogs_host(), ARG(0)); break;
    case 7: r = opeu_confirm_prompt(ds, link_dialogs_host(), ARG(0)); break;
    case 8:
        /* keyboard scratch: the 16-byte digit/mouse area of the callee's frame. */
        B2_SP_CALL(PORT_FRAME, r = opeu_choose_value(ds, link_keyboard, ARG(0), ARG(1), (int16_t)ARG(2), ARG(3),
                                                     (uint16_t)(sp - 2 - 16)));
        break;
    default: port_unhandled("chief", id, count);
    }
    return r;
}
