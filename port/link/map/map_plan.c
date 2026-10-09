/* The 15c4 map segment's shared service kinds (map_common.h). Each kind is
 * the original callee of the host branches' event; argument words in the
 * DOS declaration order of that callee:
 *   0 draw_far(x,y,w,h,mode,seg,off)          0000:28b0 opeu_draw_far
 *   1 draw_near(x,y,w,h,mode,ptr)             0000:28a8 opeu_draw_near
 *   2 bitmap_restore_near(x,y,w,h,mode,ptr)   0000:266c koei_266c
 *   3 bitmap_save_near(x,y,w,h,ptr)           0000:2e72 koei_2e72
 *   4 glyph_draw_near(x,y,w,h,color,ptr)      0000:2a81 opeu_vga_glyph
 *   5 ui_line(x1,y1,x2,y2,color)              0000:2d16 opeu_ui_line
 *   6 bitmap_restore(x,y,w,h,mode,seg,off)    0000:2674 opeu_bitmap_restore
 *   7 draw_masked_near(x,y,w,h,mode,ptr)      0000:28c5 opeu_code_0000_28c5
 *   8 sprite_compose(x,y,w,h,image off,seg,mask off,seg,buffer off,seg,flags)
 *                                             0000:3e0a opeu_sprite_compose
 *     (SYSLIB reads the image at [bp+0e], the mask at [bp+12], the buffer
 *     at [bp+16] and tests flags at [bp+1a]; MAPVIEW's DOS prototype names
 *     the last three words "mode, segment, offset", MAPOVER's "target off,
 *     target seg, mode" -- the positions are the same)
 *   9 blit_far(x,y,w,h,mode,seg,off)          0000:27ca opeu_blit_far
 *  10 pattern_draw(x,y,w,h,mode,ptr)          0000:27c2 koei_27c2
 *  11 screen_shift(x1,y1,x2,y2)               0000:3bc5 koei_3bc5
 *  12 wait_ticks(ticks)                       0a8e:004c opeu_delay_units
 *  13 control_message(text,person)            1707:0672 opeu_control_message
 *  14 status_mode(mode)                       1707:015a opeu_turn_frame
 *  15 view_prepare()                          06ac:0042 opeu_view_prepare
 *  16 read_far_bytes(off,seg,destination,size) 09cc:00a8 opeu_read_far_bytes
 * The VGALIB "far" routines take (segment, offset) on the stack
 * (vgalib.asm 2674/27ca/28b0 load DS from [bp+10], the offset from [bp+12]).
 */
#include "map_common.h"

#define W(i) ARG(i)
#define FAR(segi, offi) port_far(ARG(segi), ARG(offi))

uint16_t map_plan_service(uint8_t *ds, const char *module, uint16_t kind, const uint16_t *args, uint16_t count)
{
    switch (kind) {
    case 0: opeu_draw_far(W(0), W(1), W(2), W(3), W(4), FAR(5, 6)); break;
    case 1: opeu_draw_near(W(0), W(1), W(2), W(3), W(4), PORT_DS(W(5))); break;
    case 2: koei_266c(W(0), W(1), W(2), W(3), W(4), PORT_DS(W(5))); break;
    case 3: koei_2e72(W(0), W(1), W(2), W(3), PORT_DS(W(4))); break;
    case 4: opeu_vga_glyph(W(0), W(1), W(2), W(3), W(4), PORT_DS(W(5))); break;
    case 5: opeu_ui_line(W(0), W(1), W(2), W(3), W(4)); break;
    case 6: opeu_bitmap_restore(W(0), W(1), W(2), W(3), W(4), FAR(5, 6)); break;
    case 7: opeu_code_0000_28c5(W(0), W(1), W(2), W(3), W(4), PORT_DS(W(5))); break;
    case 8:
        opeu_sprite_compose(W(0), W(1), W(2), W(3), FAR(5, 4), FAR(7, 6), FAR(9, 8), W(10));
        break;
    case 9: opeu_blit_far(W(0), W(1), W(2), W(3), W(4), FAR(5, 6)); break;
    case 10: koei_27c2(W(0), W(1), W(2), W(3), W(4), PORT_DS(W(5))); break;
    case 11: koei_3bc5(W(0), W(1), W(2), W(3)); break;
    case 12: opeu_delay_units(ds, link_waitkey, (uint8_t)W(0)); break;
    case 13: opeu_control_message(ds, link_dialogs_host(), W(0), W(1)); break;
    case 14: opeu_turn_frame(ds, link_dialogs_host(), (uint8_t)W(0)); break;
    case 15: opeu_view_prepare(ds, link_emsmap); break;
    case 16: opeu_read_far_bytes(ds, port_far(W(1), 0), W(0), W(2), W(3)); break;
    default: port_unhandled(module, kind, count);
    }
    return 0;
}
