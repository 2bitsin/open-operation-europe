/* core/limits.c 09cc:027e min16 / 09cc:028c max16 (AX a, DX b), reached by
 * address from btlmenu. */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_min16)
{
    (void)ds;
    return opeu_min16(ARG(0), ARG(1));
}

PORT_ROUTINE(opeu_max16)
{
    (void)ds;
    return opeu_max16(ARG(0), ARG(1));
}
