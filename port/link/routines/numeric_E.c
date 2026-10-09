/* core/numeric.c, reached by address from btlcmdr and btlsetup:
 *   09f6:000a mul_div (AX a, DX b, BX denominator) -> AX
 *   09f6:004c sub_byte (AX amount, BX value pointer) -> AL
 *   09f6:0090 sub_word (AX amount, BX value pointer) -> AX
 * The host entries take host pointers into DS. */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_mul_div)
{
    (void)ds;
    return opeu_mul_div(ARG(0), ARG(1), ARG(2));
}

PORT_ROUTINE(opeu_sub_byte)
{
    (void)ds;
    return opeu_sub_byte(ARG(0), PORT_DS(ARG(1)));
}

PORT_ROUTINE(opeu_sub_word)
{
    (void)ds;
    return opeu_sub_word(ARG(0), (uint16_t *)(void *)PORT_DS(ARG(1)));
}
