/* Link: core/evreport.c (108c:1302..1594 hourly unit reports, 108c:155c
 * opeu_turn_hourly_b). Services from the host branch comment:
 * 0 pending_finish(value), 1 message_lookup(id) -> text offset,
 * 2 unit_message(unit,text), 3 daily_show(id,side,callback offset,callback
 * segment), 4 daily_context(side), 5 far/near event call(segment,offset). */
#include "c1_common.h"

uint16_t link_evreport(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;               /* 0a8e:005c */
    case 1: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;           /* 312c:000c */
    case 2: opeu_unit_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break;            /* 1707:07ec unit_message */
    case 3:                                                                               /* 1707:03e4 */
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1), (uint32_t)ARG(3) << 16 | ARG(2));
        break;
    case 4: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;                /* 1389:000e */
    case 5: port_call(ds, (uint32_t)ARG(0) * 16u + ARG(1), NULL, 0); break;              /* segment, offset */
    default: port_unhandled("evreport", id, count);
    }
    return r;
}
