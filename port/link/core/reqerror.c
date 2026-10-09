/* Link: core/reqerror.c (1270:02e4 request-failure presentation).
 * Services (host branch macros): 1 message_lookup(id) 312c:000c -> text,
 * 2 request_format(dest,format,...) 0480:0b9a, 3 request_message(text,person)
 * = control_message 1707:0672, 4 pending_prepare 0d92:0070,
 * 5 pending_finish(value) 0a8e:005c. */
#include "c2_common.h"

uint16_t link_reqerror(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 1: r = opeu_message_lookup(ds, link_msglook, (short)ARG(0)); break;       /* 312c:000c */
    case 2: c2_request_format(ds, args, count); break;                            /* 0480:0b9a */
    case 3: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break; /* 1707:0672 */
    case 4: opeu_pending_prepare(ds, link_soundcue); break;                       /* 0d92:0070 */
    case 5: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;        /* 0a8e:005c */
    default: port_unhandled("reqerror", id, count);
    }
    return r;
}
