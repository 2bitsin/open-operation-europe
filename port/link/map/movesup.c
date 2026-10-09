/* Link: map/movesup.c (32e6:0288 supplied, 02e4 consume, 0366 sighted).
 * Events (host branch comment; callees from the DOS branch):
 *   0 min16(a,b)               NUMERIC opeu_min16
 *   1 max16(a,b)               NUMERIC opeu_max16
 *   2 sub_word(amount,value*)  NUMERIC opeu_sub_word; value* is the DS
 *                              offset of the word it updates */
#include "map_common.h"

uint16_t link_movesup(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0, value;
    switch (id) {
    case 0: r = opeu_min16(ARG(0), ARG(1)); break;
    case 1: r = opeu_max16(ARG(0), ARG(1)); break;
    case 2:
        value = (uint16_t)(ds[ARG(1)] | ds[(uint16_t)(ARG(1) + 1u)] << 8);
        r = opeu_sub_word(ARG(0), &value);
        ds[ARG(1)] = (uint8_t)value;
        ds[(uint16_t)(ARG(1) + 1u)] = (uint8_t)(value >> 8);
        break;
    default: port_unhandled("movesup", id, count);
    }
    return r;
}
