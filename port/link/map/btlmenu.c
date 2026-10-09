/* Link: map/btlmenu.c, address-keyed BattleEvent: player orders segment 2ec8 (hex mask reads, see btl_orders_event in map/btl_link.h). */
#include "map/btl_link.h"

uint16_t link_btlmenu(uint8_t *ds, uint32_t service, const uint16_t *args, unsigned int count)
{
    return btl_orders_event(ds, service, args, count);
}
