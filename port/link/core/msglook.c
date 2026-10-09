/* Link: core/msglook.c (312c:000c opeu_message_lookup). Events, from the
 * host branch's comment:
 *   0 daily_context(side) 1389:000e opeu_daily_context
 *   1 string_copy_far(DS:e668, table offset, table segment) 09cc:00f4
 *   2 request_format(DS:e668, DS:bc86) 0480:0b9a */
#include "b1_util.h"

uint16_t link_msglook(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;
    case 1: r = opeu_string_copy_far(ds, ARG(0), port_far(ARG(2), 0), ARG(1)); break;
    case 2: b1_request_format(ds, args, count); break;
    default: port_unhandled("msglook", id, count);
    }
    return r;
}
