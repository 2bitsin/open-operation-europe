/* Link: map/moveunit.c (32e6:0732 move the current unit toward its order
 * destination). Its MoveEvent ids are the movement file's shared ids
 * (map_move.c); MOVEUNIT raises 0..24.
 *
 * Id 5, 32e6:00d8 opeu_move_reachable (REACHABLE(x,y,tx,ty,unit) in
 * src/main/map/moveunit.c), is called when the compiled reference has its
 * host branch (OPEU_HAVE_MOVE_REACHABLE); otherwise map_move.c stops with
 * a fatal message when it is raised. */
#include "map_common.h"

uint16_t link_moveunit(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    if (id > 24)
        port_unhandled("moveunit", id, count);
    return map_move_service(ds, "moveunit", id, args, count);
}
