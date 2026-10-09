/* Link: map/btlcalc.c (2864, battle calculations). BattleService ids of the
 * host branch (src/main/map/btlcalc.c WAIT / ELEMENT_SIDE / UNIT_SIDE /
 * ELEMENT_UNIT; tests/validate_battle_calc.py):
 *   0 wait(ticks)         0a8e:004c delay_units (WAITKEY)
 *   1 element_side(e)     2864:000e battle_element_side (BTLLIST)
 *   2 unit_side(u)        2864:001c battle_unit_side (BTLLIST)
 *   3 element_unit(e)     2864:002c battle_element_unit (BTLLIST) */
#include "map/btl_link.h"

uint16_t link_btlcalc(uint8_t *ds, uint16_t id, uint16_t arg)
{
    switch (id) {
    case 0: opeu_delay_units(ds, link_waitkey, (uint8_t)arg); return 0;
    case 1: return opeu_battle_element_side(ds, link_btllist, arg);
    case 2: return opeu_battle_unit_side(ds, link_btllist, arg);
    case 3: return opeu_battle_element_unit(ds, link_btllist, arg);
    default: port_unhandled("btlcalc", id, 1); return 0;
    }
}
