/* Link: io/listui.c (0a8e:006a..0817 list windows). The shared list
 * boundary of LISTUI and LISTDLG (host-branch comment of listui.c):
 *   0 draw_far(x,y,w,h,mode,seg,off) 0000:28b0
 *   1 outline(x1,y1,x2,y2,color) 0a04:065a opeu_ui_outline
 *   2 mouse_show 05a8:003e   3 mouse_hide 05a8:0062
 *   4 mouse_position 05a8:0086 (stores x, y in args[0], args[1])
 *   5 mouse_buttons 05a8:0170 -> AL   6 input_reset 05a8:01b4
 *   7 highlight(x,y,w,h,color) 0000:2b0e   8 line 0000:2d16
 *   9 screen_copy(6 words) 0000:301f   10 box 0000:2ad2
 *  11 bitmap_save(x,y,w,h,seg,off) 0000:2ed8
 *  12 bitmap_restore(x,y,w,h,mode,seg,off) 0000:2674
 *  13 paper(value) 0480:0838 opeu_text_paper_set
 *  14 text_color(value) 0480:082e   15 position(x,y) 0480:0808
 *  16 print(format, 7 words) 0480:0bb8 opeu_cursor_print
 *  17 keyboard_key 0541:00ba -> AL   18 yes_no(text) 0541:0664 -> AX
 *  19 draw callback(off,seg,al,dl,bl,s1,s0): far row-draw routine read from
 *     the window record -> port_call (AX,DX,BX, then stack words)
 *  20 select callback(off,seg,al) -> AL: far select routine -> port_call. */
#include "b1_util.h"

#define KEY_SCRATCH 16u   /* keyboard.c: 16-byte caller scratch area */

uint16_t link_listui(uint8_t *ds, uint16_t kind, uint16_t *args, uint16_t count)
{
    uint16_t r = 0, s;
    switch (kind) {
    case 0: r = (uint16_t)opeu_draw_far(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6))); break;
    case 1: opeu_ui_outline(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 2: opeu_mouse_show(ds, link_mouse); break;
    case 3: opeu_mouse_hide(ds, link_mouse); break;
    case 4:
        s = b1_stack_alloc(4);
        opeu_mouse_position(ds, link_mouse, s, (uint16_t)(s + 2));
        if (count >= 2) {
            args[0] = port_ds_word(s);
            args[1] = port_ds_word((uint16_t)(s + 2));
        }
        b1_stack_free(4);
        break;
    case 5: r = opeu_mouse_buttons(ds, link_mouse); break;
    case 6: opeu_input_reset(ds, link_mouse); break;
    case 7: r = (uint16_t)opeu_ui_highlight(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 8: opeu_ui_line(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 9: opeu_screen_copy(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5)); break;
    case 10: r = (uint16_t)opeu_ui_box(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 11: opeu_bitmap_save(ARG(0), ARG(1), ARG(2), ARG(3), port_far(ARG(4), ARG(5))); break;
    case 12:
        r = (uint16_t)opeu_bitmap_restore(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
        break;
    case 13: opeu_text_paper_set(ds, link_textout_host(), (uint8_t)ARG(0)); break;
    case 14: opeu_text_color(ds, link_textout_host(), ARG(0)); break;
    case 15: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 16: b1_cursor_print(ds, args, count); break;
    case 17: r = opeu_keyboard_key(ds, link_keyboard); break;
    case 18:
        s = b1_stack_alloc(KEY_SCRATCH);
        r = (uint16_t)opeu_prompt_yes_no(ds, link_keyboard, ARG(0), s);
        b1_stack_free(KEY_SCRATCH);
        break;
    case 19:
        port_call(ds, (uint32_t)ARG(1) * 16u + ARG(0), args + 2, count >= 2 ? count - 2u : 0u);
        break;
    case 20:
        r = (uint8_t)port_call(ds, (uint32_t)ARG(1) * 16u + ARG(0), args + 2, count >= 2 ? count - 2u : 0u);
        break;
    default: port_unhandled("listui", kind, count);
    }
    return r;
}
