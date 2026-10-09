/* Link: core/evbomb.c (108c:094a..0c4e air-raid event). Services from the
 * host branch comment:
 *  0 bitmap_save(x,y,w,h,segment,offset)
 *  1 draw_far(x,y,w,h,mode,segment,offset)
 *  2 sprite_compose 0000:3e0a(x,y,w,h,image off,seg,mask off,seg,target off,seg,mode)
 *  3 bitmap_restore(x,y,w,h,mode,segment,offset)
 *  4 15c4:13f6(x,y,side)   5 15c4:055c(x pointer,y pointer)
 *  6 sound_cue4()  7 input_delay(ticks)  8 music_command7()  9 pending_finish(v)
 * 10 unit_name(unit,buffer) 11 message_lookup(id) -> text offset
 * 12 request_format(destination,format,argument) 13 control_message(text,person)
 * 14 daily_show(id,side,callback offset,callback segment)
 * 15 unit_casualties(unit,percent,kind,c,d,e)
 * The routines' `load` parameter is 0: the port keeps the original segment
 * numbering, so far segments are used as they are. */
#include "c1_common.h"

uint16_t link_evbomb(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0:                                                                                /* SYSLIB 2ed8 */
        opeu_bitmap_save(ARG(0), ARG(1), ARG(2), ARG(3), port_far(ARG(4), ARG(5)));
        break;
    case 1:                                                                                /* SYSLIB 28b0 */
        r = (uint16_t)opeu_draw_far(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
        break;
    case 2:                                                                                /* SYSLIB 3e0a */
        opeu_sprite_compose(ARG(0), ARG(1), ARG(2), ARG(3), port_far(ARG(5), ARG(4)),
                            port_far(ARG(7), ARG(6)), port_far(ARG(9), ARG(8)), ARG(10));
        break;
    case 3:                                                                                /* SYSLIB 2674 */
        r = (uint16_t)opeu_bitmap_restore(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
        break;
    case 4:                                                                                /* 15c4:13f6 */
        frame = c1_frame_enter();
        opeu_map_center(ds, link_mappos, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c1_frame_leave();
        break;
    case 5: opeu_view_pixel(ds, ARG(0), ARG(1)); break;                                   /* 15c4:055c */
    case 6: opeu_sound_cue4(ds, link_soundcue); break;                                    /* 0d92:0028 */
    case 7: opeu_delay_ticks(ds, link_waitkey, ARG(0)); break;                            /* 0a8e:000c */
    case 8: opeu_music_command7(ds, link_sound); break;                                   /* 0d65:01b0 */
    case 9: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;                /* 0a8e:005c */
    case 10: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;                /* 0b73:0188 */
    case 11: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;           /* 312c:000c */
    case 12: c1_request_format(ds, args, count); break;                                   /* 0480:0b9a */
    case 13: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;       /* 1707:0672 */
    case 14: c1_daily_show(ds, ARG(0), ARG(1), ARG(2), ARG(3)); break;                    /* 1707:03e4 */
    case 15:                                                                               /* 17c4:0b1c */
        frame = c1_frame_enter();
        opeu_unit_casualties(ds, link_casualty, frame, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
        c1_frame_leave();
        break;
    default: port_unhandled("evbomb", id, count);
    }
    return r;
}
