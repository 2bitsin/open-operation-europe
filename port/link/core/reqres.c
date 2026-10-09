/* Link: core/reqres.c (1270:05ac request action 0, resource delivery).
 * Services (host branch macros): 0 arrival_location(x,y,side) 15c4:1408,
 * 1 message_lookup(id) 312c:000c -> text, 2 request_format(dest,format,...)
 * 0480:0b9a, 3 request_message(text,person) = control_message 1707:0672,
 * 4 request_wait 0d92:0060. */
#include "c2_common.h"

uint16_t link_reqres(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0:                                                           /* 15c4:1408 */
        frame = c2_frame_enter();
        opeu_arrival_location(ds, link_mappos, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c2_frame_leave();
        break;
    case 1: r = opeu_message_lookup(ds, link_msglook, (short)ARG(0)); break;       /* 312c:000c */
    case 2: c2_request_format(ds, args, count); break;                            /* 0480:0b9a */
    case 3: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break; /* 1707:0672 */
    case 4: opeu_request_wait(ds, link_soundcue); break;                          /* 0d92:0060 */
    default: port_unhandled("reqres", id, count);
    }
    return r;
}
