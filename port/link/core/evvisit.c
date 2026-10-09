/* Link: core/evvisit.c (108c:000e..0395 the commander-visit event).
 * Services from the host branch comment:
 *   0 message_lookup(id)->text, 1 request_format(destination,format,value),
 *   2 control_message(text,person), 3 daily_context(side),
 *   4 set_turn_context(value), 5 mark 15c4:087e(x,y,side),
 *   6 daily_show(id,side,presenter offset,presenter segment),
 *   7 finish_side_initialization(value). */
#include "c1_common.h"

uint16_t link_evvisit(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;           /* 312c:000c */
    case 1: c1_request_format(ds, args, count); break;                                   /* 0480:0b9a */
    case 2: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;        /* 1707:0672 */
    case 3: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;                /* 1389:000e */
    case 4: opeu_set_turn_context(ds, link_sound, (uint8_t)ARG(0)); break;               /* 0d65:00fe */
    case 5:                                                                               /* 15c4:087e */
        frame = c1_frame_enter();
        opeu_location_flash(ds, link_mapunit, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c1_frame_leave();
        break;
    case 6: c1_daily_show(ds, ARG(0), ARG(1), ARG(2), ARG(3)); break;                    /* 1707:03e4 */
    case 7: opeu_finish_side_initialization(ds, link_sound, (uint8_t)ARG(0)); break;     /* 0d65:0032 */
    default: port_unhandled("evvisit", id, count);
    }
    return r;
}
