/* Link: core/evpartis.c (108c:0396 hurricane presenter, 108c:0402 partisan
 * event). Services from the host branch comment:
 * 0 sound_cue15(), 1 daily_context(side), 2 message_lookup(id) -> text,
 * 3 request_format(destination,format,arguments...) (cdecl),
 * 4 control_message(text,person), 5 daily_show(id,side,callback offset,
 * callback segment), 6 15c4:087e location_flash(x,y,side), 7 map_erase(x,y). */
#include "c1_common.h"

uint16_t link_evpartis(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0: opeu_sound_cue15(ds, link_soundcue); break;                                  /* 0d92:0080 */
    case 1: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;                /* 1389:000e */
    case 2: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;           /* 312c:000c */
    case 3: c1_request_format(ds, args, count); break;                                   /* 0480:0b9a */
    case 4: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;        /* 1707:0672 */
    case 5: c1_daily_show(ds, ARG(0), ARG(1), ARG(2), ARG(3)); break;                    /* 1707:03e4 */
    case 6:                                                                               /* 15c4:087e */
        frame = c1_frame_enter();
        opeu_location_flash(ds, link_mapunit, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c1_frame_leave();
        break;
    case 7:                                                                               /* 15c4:0f7c */
        frame = c1_frame_enter();
        opeu_map_erase(ds, link_maperase, frame, (uint8_t)ARG(0), (uint8_t)ARG(1));
        c1_frame_leave();
        break;
    default: port_unhandled("evpartis", id, count);
    }
    return r;
}
