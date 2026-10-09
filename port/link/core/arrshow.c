/* Link: core/arrshow.c (1270:0816 opeu_arrival_present). Services, from
 * the host branch and tests/validate_arrival_presentation.py SERVICES:
 * 1 message_lookup(id), 2 request_format(destination, format, ...) (cdecl;
 * also issued by units/unitname.c, which receives this callback),
 * 3 request_message(text, person), 4 request_wait, 5 daily_context(side). */
#include "c1_common.h"

uint16_t link_arrshow(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 1: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;     /* 312c:000c */
    case 2: c1_request_format(ds, args, count); break;                             /* 0480:0b9a */
    case 3: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;  /* 1707:0672 */
    case 4: opeu_request_wait(ds, link_soundcue); break;                           /* 0d92:0060 */
    case 5: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;          /* 1389:000e */
    default: port_unhandled("arrshow", id, count);
    }
    return r;
}
