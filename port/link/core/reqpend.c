/* Link: core/reqpend.c (1270:0430..05ab pending-request start/fallback).
 * Services (host branch macros, word arguments a,b,c,d):
 * 0 daily_show(id, side, present offset, present segment) 1707:03e4,
 * 1 message_lookup(id) 312c:000c -> text, 2 request_format(dest,format,name)
 * 0480:0b9a, 3 request_message(text,person) = control_message 1707:0672,
 * 4 pending_prepare 0d92:0070, 5 pending_finish(value) 0a8e:005c. */
#include "c2_common.h"

uint16_t link_reqpend(uint8_t *ds, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    uint16_t words[3];
    uint16_t r = 0;
    switch (id) {
    case 0:                                                           /* 1707:03e4 */
        opeu_daily_show(ds, link_dialogs_host(), a, (uint8_t)b, (uint32_t)d << 16 | c);
        break;
    case 1: r = opeu_message_lookup(ds, link_msglook, (short)a); break;           /* 312c:000c */
    case 2:                                                           /* 0480:0b9a */
        words[0] = a; words[1] = b; words[2] = c;
        c2_request_format(ds, words, 3);
        break;
    case 3: opeu_control_message(ds, link_dialogs_host(), a, b); break;           /* 1707:0672 */
    case 4: opeu_pending_prepare(ds, link_soundcue); break;                       /* 0d92:0070 */
    case 5: opeu_pending_finish(ds, link_waitkey, (uint8_t)a); break;             /* 0a8e:005c */
    default: port_unhandled("reqpend", id, 4);
    }
    return r;
}
