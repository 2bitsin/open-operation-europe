/* Link: io/menudraw.c (05a8:0266..0573 menu rendering and mouse selection).
 * The module's service is the shared 05a8 input boundary (host-branch
 * comment, src/main/io/menudraw.c "Host model"):
 *   1 int86(0x33) with seven REGS words: MOUSE's boundary (link_mouse 1)
 *   2 context save 0480:0844      3 position(x,y) 0480:0808
 *   4 print(format,char,label) 0480:0bb8 (opeu_context_print, host opeu_cursor_print)
 *   5 context restore 0480:086a   6 tile blit(x,y,w,h,seg,off) 0000:2e57
 *   7 bitmap save(x,y,w,h,seg,off) 0000:2ed8
 *   8 bitmap restore(x,y,w,h,mode,seg,off) 0000:2674
 *   9 delay(units) 0a8e:004c opeu_delay_units */
#include "b1_util.h"

uint16_t link_menudraw(uint8_t *ds, uint16_t call, uint16_t *words)
{
    switch (call) {
    case 1: return link_mouse(ds, 1, words);
    case 2: opeu_context_save(ds, link_textout_host()); break;
    case 3: opeu_army_position(ds, link_textout_host(), (int16_t)words[0], (int16_t)words[1]); break;
    case 4: b1_cursor_print(ds, words, 3); break;
    case 5: opeu_context_restore(ds, link_textout_host()); break;
    case 6: koei_2e57(words[0], words[1], words[2], words[3], port_far(words[4], words[5])); break;
    case 7: opeu_bitmap_save(words[0], words[1], words[2], words[3], port_far(words[4], words[5])); break;
    case 8:
        opeu_bitmap_restore(words[0], words[1], words[2], words[3], words[4], port_far(words[5], words[6]));
        break;
    case 9: opeu_delay_units(ds, link_waitkey, (uint8_t)words[0]); break;
    default: port_unhandled("menudraw", call, 7);
    }
    return 0;
}
