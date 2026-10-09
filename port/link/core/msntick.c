/* Link: core/msntick.c (0e8e:0a78..0b5d mission countdown, hourly step A).
 * Services 14 weather_update, 15 turn_prepare, 16 air_operations, 17 the
 * mission handler from the DS:7a54 far table (c2_msn.h). */
#include "c2_msn.h"

uint16_t link_msntick(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_msn_service("msntick", ds, id, args, count);
}
