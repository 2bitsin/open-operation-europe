/* Link: map/dplunit.c, address-keyed BattleEvent: deployment group (source-order words, see btl_deploy_event in map/btl_link.h). */
#include "map/btl_link.h"

uint16_t link_dplunit(uint8_t *ds, uint32_t service, const uint16_t *args, unsigned int count)
{
    return btl_deploy_event(ds, service, args, count);
}
