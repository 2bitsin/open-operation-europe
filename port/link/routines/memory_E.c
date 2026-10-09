/* core/memory.c 09cc:00a8 read_far_bytes (AX size, BX destination, stack:
 * far source offset, segment), reached by address from btlicon. The host
 * entry takes the source segment as a host base pointer. */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_read_far_bytes)
{
    opeu_read_far_bytes(ds, port_far(ARG(3), 0), ARG(2), ARG(1), ARG(0));
    return 0;
}
