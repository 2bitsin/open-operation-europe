/* core/random.c 09f6:00d4 rand_bounded (AX bound) -> AX, reached by address
 * from the battle modules. The recovered generator keeps its state in a
 * global; the original state is the dword at DS:c6f4, copied in and out
 * around the call as the other link files do. */
#include <string.h>
#include "map/btl_link.h"

extern uint32_t opeu_random_state;

PORT_ROUTINE(opeu_rand_bounded)
{
    uint16_t r;
    memcpy(&opeu_random_state, ds + 0xc6f4, 4);
    r = opeu_rand_bounded(ARG(0));
    memcpy(ds + 0xc6f4, &opeu_random_state, 4);
    return r;
}
