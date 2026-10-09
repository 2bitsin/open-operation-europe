/* Link: core/evangel.c (108c:0c4e..0fd7 Sister Angelique's visit).
 * Services from the host branch comment:
 * 0 message panel (1707:0346), 1 frame(x,y,width), 2 print(format),
 * 3 seek(handle,offset low,offset high,origin), 4 draw(x,y,w,h,mode,handle),
 * 5 lookup(id) -> text, 6 format(buffer,format,text), 7 position(x,y),
 * 8 text(buffer), 9 arrival_location(x,y,side), 10 15c4:087e(x,y,side),
 * 11 daily_show(id,side,callback offset,callback segment),
 * 12 far action through DS:7cec other than this module's own (person,unit). */
#include "c1_common.h"

uint16_t link_evangel(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0: opeu_message_panel_begin(ds, link_dialogs_host()); break;                    /* 1707:0346 */
    case 1: opeu_context_frame(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;   /* 0a04:02a0 */
    case 2: c1_cursor_print(ds, args, count); break;                                     /* 0480:0bb8 army_print */
    case 3: {                                                                             /* FILEIO 009b dos_seek */
        int32_t position = opeu_dos_seek((int16_t)ARG(0), (int32_t)ARG32(1), (int16_t)ARG(3));
        r = (uint16_t)position;
        break;
    }
    case 4:                                                                               /* 0a04:000c */
        opeu_image_read_draw(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), (int16_t)ARG(5));
        break;
    case 5: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;           /* 312c:000c */
    case 6: c1_request_format(ds, args, count); break;                                   /* 0480:0b9a */
    case 7: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break; /* 0480:0808 */
    case 8: opeu_text_feed(ds, link_dialogs_host(), ARG(0)); break;                      /* 1707:0652 message_text */
    case 9:                                                                               /* 15c4:1408 */
        frame = c1_frame_enter();
        opeu_arrival_location(ds, link_mappos, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c1_frame_leave();
        break;
    case 10:                                                                              /* 15c4:087e */
        frame = c1_frame_enter();
        opeu_location_flash(ds, link_mapunit, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c1_frame_leave();
        break;
    case 11: c1_daily_show(ds, ARG(0), ARG(1), ARG(2), ARG(3)); break;                   /* 1707:03e4 */
    case 12:
        /* The host branch passes only (person, unit), not the table entry's
         * address (evangel.c host_action), so the target cannot be called.
         * The DGEVENTS table at DS:7cec holds only 108c actions, which the
         * host branch runs itself. */
        port_fatal("evangel: far action outside 108c without its address (person %04x unit %04x)", ARG(0), ARG(1));
        break;
    default: port_unhandled("evangel", id, count);
    }
    return r;
}
