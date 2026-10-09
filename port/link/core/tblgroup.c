/* Link: core/tblgroup.c (0c45:0002/01e6 division and mission row-draw
 * callbacks). Services: the TableEvent protocol, ids 0..7
 * (b2_table_service in b2_common.h). */
#include "b2_common.h"

uint16_t link_tblgroup(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return b2_table_service(ds, "tblgroup", id, args, count);
}
