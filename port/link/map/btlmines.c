/* Link: map/btlmines.c, address-keyed MineEvent, prototype-order words (see btl_mines_event in map/btl_link.h). */
#include "map/btl_link.h"

uint16_t link_btlmines(uint8_t *ds, uint32_t service, const uint16_t *args, unsigned int count)
{
    return btl_mines_event(ds, service, args, count);
}
