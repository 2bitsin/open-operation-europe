/* Link: map/movetgt.c (32e6:0a92 adjacent cell, 0ae8 objective target, 0b38
 * unit target, 0c00/0c3e level counters). Its MoveEvent ids are the
 * movement file's shared ids (map_move.c); MOVETGT raises 1, 4, 9..12 and
 * 25..30. */
#include "map_common.h"

uint16_t link_movetgt(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return map_move_service(ds, "movetgt", id, args, count);
}
