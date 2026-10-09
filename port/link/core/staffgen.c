/* Link: core/staffgen.c (06db:0c20..0d64 candidate person attributes).
 * Services, from the host branch: portrait=0 (x, y, person offset) ->
 * 0a04:046c opeu_context_portrait. */
#include "b2_common.h"

void link_staffgen(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: opeu_context_portrait(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    default: port_unhandled("staffgen", id, count);
    }
}
