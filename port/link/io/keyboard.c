/* Link: io/keyboard.c (0541:0008..067b keyboard waits, cursor blink,
 * decimal entry, value choice, yes/no prompts).
 *
 * The service is the shared 05a8 input boundary. KEYBOARD passes it to the
 * MOUSE, MENUDRAW and NUMDLG routines it calls, so it answers their calls
 * too (forwarded to their link callbacks, which own those ids):
 *   1      int86(0x33)                                   -> link_mouse
 *   2..9   MENUDRAW calls (context save/restore, position, print, tile
 *          blit, bitmap save/restore, delay units)       -> link_menudraw
 *   10..13 NUMDLG calls (ui_fill, ui_invert, EMS dialog map, ui_line)
 *                                                          -> link_numdlg
 * and its own (src/main/io/keyboard.c "Host model"; addresses checked
 * against tests/validate_keyboard.py SERVICES):
 *   20 read_key -> key       0480:0002 opeu_key_read (TEXTOUT)
 *   21 input_flush           0480:00a4 opeu_input_flush
 *   22 text_home(y)          0480:0826 opeu_text_set_y
 *   23 text_putc(c)          0480:0944 opeu_text_put
 *   24 text_backspace        0480:0bfa opeu_text_erase_char
 *   25 timer_start(ticks)    0000:4052 opeu_fm_wait_start
 *   26 timer_running -> word 0000:405f opeu_fm_busy
 *   27 turn_input_b -> word  0000:4066 opeu_turn_input_b
 *   28 sprite_draw(x,y,w,h, image off,seg, mask off,seg, buffer off,seg, flags)
 *                            0000:3f26 koei_3f26
 *   29 draw_far(x,y,w,h,mode,seg,off) 0000:28b0 opeu_draw_far
 *   30 ui_invert(x,y,w,h,c)  0000:2b0e opeu_ui_highlight
 *   31 delay_ticks(n)        0a8e:000c opeu_delay_ticks (WAITKEY)
 *   32 pending_finish(n)     0a8e:005c opeu_pending_finish
 *   33 army_print(format,text) 0480:0bb8 opeu_cursor_print (cdecl frame)
 *   34 foreign idle hook (offset, segment): the far routine at DS:3eba,
 *      called through the routine link table by its address.
 */
#include "a_util.h"

uint16_t link_keyboard(uint8_t *ds, uint16_t call, uint16_t *w)
{
    switch (call) {
    case 1: return link_mouse(ds, call, w);
    case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
        return link_menudraw(ds, call, w);
    case 10: case 11: case 12: case 13:
        return link_numdlg(ds, call, w);
    case 20: return opeu_key_read(ds, link_textout_host());
    case 21: opeu_input_flush(ds, link_textout_host()); break;
    case 22: opeu_text_set_y(ds, link_textout_host(), (int16_t)w[0]); break;
    case 23: opeu_text_put(ds, link_textout_host(), (uint8_t)w[0]); break;
    case 24: opeu_text_erase_char(ds, link_textout_host()); break;
    case 25: opeu_fm_wait_start(w[0]); break;
    case 26: return opeu_fm_busy();
    case 27: return opeu_turn_input_b();
    case 28:
        koei_3f26(w[0], w[1], w[2], w[3], port_far(w[5], w[4]), port_far(w[7], w[6]),
                  port_far(w[9], w[8]), w[10]);
        break;
    case 29: opeu_draw_far(w[0], w[1], w[2], w[3], w[4], port_far(w[5], w[6])); break;
    case 30: opeu_ui_highlight(w[0], w[1], w[2], w[3], w[4]); break;
    case 31: opeu_delay_ticks(ds, link_waitkey, w[0]); break;
    case 32: opeu_pending_finish(ds, link_waitkey, (uint8_t)w[0]); break;
    case 33: a_cursor_print(ds, w, 2); break;
    case 34: port_call(ds, (uint32_t)w[1] * 16u + w[0], 0, 0); break;
    default: port_unhandled("keyboard", call, 0);
    }
    return 0;
}
