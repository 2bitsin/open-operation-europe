/* Link: core/evcomms.c (11e5:01a4 hourly objective events). Services from
 * the host branch comment: 4 control_message(text,person),
 * 5 request_format(destination,format,objective) (cdecl),
 * 6 location_flash(x,y,side). Ids 0..3 are not used. */
#include "c1_common.h"

uint16_t link_evcomms(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame;
    switch (id) {
    case 4: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;        /* 1707:0672 */
    case 5: c1_request_format(ds, args, count); break;                                   /* 0480:0b9a */
    case 6:                                                                               /* 15c4:087e */
        frame = c1_frame_enter();
        opeu_location_flash(ds, link_mapunit, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c1_frame_leave();
        break;
    default: port_unhandled("evcomms", id, count);
    }
    return 0;
}
