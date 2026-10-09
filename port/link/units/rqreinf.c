/* Link: units/rqreinf.c (22f9:0620 reinforcement element, 22f9:0780
 * regiment request). Services from the host branch's table (request-menu
 * numbering, rq_F.h): 0..18; opeu_reinf_element receives this module's
 * event and raises 0, 1, 2 through it. */
#include "rq_F.h"

uint16_t link_rqreinf(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    if (id <= 18)
        return lf_rq_service("rqreinf", ds, id, args, count);
    port_unhandled("rqreinf", id, count);
    return 0;
}
