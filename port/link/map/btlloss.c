/* Link: map/btlloss.c, address-keyed BattleEvent: commander group (BX-before-AX words, see btl_commander_event in map/btl_link.h). */
#include "map/btl_link.h"

uint16_t link_btlloss(uint8_t *ds, uint32_t service, const uint16_t *args, unsigned int count)
{
    return btl_commander_event(ds, service, args, count);
}
