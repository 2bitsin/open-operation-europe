/* Link: map/influenc.c (1270:0d8e influence map, 1270:0efc hourly stage C
 * entry). Events (host branch STAGE_B/STAGE_C):
 *   0 hourly_c_stage_b()   1270:08e0 opeu_hourly_c_stage_b (REQUESTS)
 *   1 hourly_c_stage_c()   1270:0c32 opeu_hourly_c_stage_c (ARRIVALS)
 * REQUESTS' host entry takes `stale_threshold`, the word the original reads
 * from its uninitialized threshold local (src/main/core/requests.c "Host
 * model"; tests/validate_requests_arrivals.py places it at entry SP - 10h).
 * It is passed from that slot of the modeled DGROUP stack. */
#include "map_common.h"

uint16_t link_influenc(uint8_t *ds, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    uint16_t sp;
    (void)a; (void)b; (void)c; (void)d;
    switch (id) {
    case 0:                                                         /* 1270:08e0 */
        sp = port_stack_enter(PORT_FRAME);
        opeu_hourly_c_stage_b(ds, link_requests, port_ds_word((uint16_t)(sp - 0x10u)));
        port_stack_leave(PORT_FRAME);
        break;
    case 1: opeu_hourly_c_stage_c(ds, link_arrivals); break;        /* 1270:0c32 */
    default: port_unhandled("influenc", id, 0);
    }
    return 0;
}
