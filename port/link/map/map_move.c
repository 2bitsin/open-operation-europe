/* The movement source file's shared service ids (MOVEUNIT 32e6:0732 and
 * MOVETGT 32e6:0a92..0c7e: "ids of tests/validate_move_unit.py", whose
 * SERVICES table names each callee address). Pointer words are DS offsets;
 * byte arguments arrive zero-extended; byte results use the low byte.
 *   0 move_rating             32e6:06ba (MOVEMINE)
 *   1 grid_step(x*,y*,dir)    145c:0d3e
 *   2 move_cost(x,y,unit)     32e6:0008
 *   3 grid_distance(x,y,x2,y2) 17c4:1424
 *   4 unit_order_destination(unit,x*,y*) 145c:02ea
 *   5 move_reachable(x,y,tx,ty,unit)     32e6:00d8 (host branch when OPEU_HAVE_MOVE_REACHABLE)
 *   6 category_6(code)        0b73:004a
 *   7 grid_step_toward(x*,y*,tx,ty) 17c4:1346 (path grid FAR_DATA 4582)
 *   8 move_clear(x,y)         32e6:00bc (path grid FAR_DATA 4582)
 *   9 find_objective_at(x,y)  145c:016a
 *  10 objective_side(obj)     145c:01b8
 *  11 find_unit_at(x,y)       145c:011c
 *  12 unit_side(unit)         145c:006c
 *  13 cell_side(x,y)          145c:0e42
 *  14 move_supplied()         32e6:0288 (MOVESUP)
 *  15 arrival_location(x,y,side) 15c4:1408 (MAPPOS)
 *  16 arrival_view(unit)      15c4:0808 (MAPUNIT)
 *  17 unit_erase(unit)        15c4:0fe4 opeu_unit_remove_draw (MAPERASE)
 *  18 arrival_prepare(unit)   15c4:0fd0 (MAPERASE)
 *  19 move_clamp()            32e6:0686 (MOVEMINE)
 *  20 cell_marked(x,y,side)   145c:0eb2
 *  21 move_mines(x,y)         32e6:05a8 (MOVEMINE)
 *  22 move_spot()             32e6:0368 (MOVESPOT)
 *  23 move_consume()          32e6:02e4 (MOVESUP)
 *  24 move_enemy_adjacent()   32e6:051e (MOVESPOT)
 *  25 restore_unit_order(unit) 145c:02b4
 *  26 unit_selection_eligible(unit) 145c:0fc4
 *  27 unit_special(unit)      145c:04de
 *  28 unit_name(unit,buffer)  0b73:0188 (UNITNAME)
 *  29 request_format(dest,fmt,v,s,n) 0480:0b9a (TEXTOUT, sprintf)
 *  30 unit_message(unit,text) 1707:07ec opeu_unit_report (DIALOGS)
 */
#include "map_common.h"

#if OPEU_HAVE_MOVE_REACHABLE
uint16_t opeu_move_reachable(uint8_t ds[65536], uint16_t *far_words, uint8_t x, uint8_t y,
                             uint8_t tx, uint8_t ty, uint16_t unit);
#endif

uint16_t map_move_service(uint8_t *ds, const char *module, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0: r = opeu_move_rating(ds, link_movemine); break;
    case 1: r = opeu_grid_step(PORT_DS(ARG(0)), PORT_DS(ARG(1)), (uint8_t)ARG(2)); break;
    case 2: r = opeu_move_cost(ds, (uint8_t)ARG(0), (uint8_t)ARG(1), ARG(2)); break;
    case 3: r = opeu_grid_distance((uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3)); break;
    case 4: r = opeu_unit_order_destination(ds, ARG(0), PORT_DS(ARG(1)), PORT_DS(ARG(2))); break;
    case 5:
#if OPEU_HAVE_MOVE_REACHABLE
        /* The host branch takes FAR_DATA 4582 as host-order words (queue
         * from word 0, grid from word 0x640); the arena holds them in the
         * original little-endian order, which is host order on every
         * supported target. */
        r = opeu_move_reachable(ds, (uint16_t *)port_far(MAP_PATH_SEGMENT, 0), (uint8_t)ARG(0),
                                (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3), ARG(4));
#else
        /* 32e6:00d8 opeu_move_reachable exists only in the DOS branch of
         * this reference's src/main/map/movecost.c (no host branch). */
        port_fatal("%s: service 5 is 32e6:00d8 move_reachable, which has no host branch "
                   "(src/main/map/movecost.c); not linkable", module);
#endif
        break;
    case 6: r = opeu_category_6((uint8_t)ARG(0)); break;
    case 7:
        r = opeu_grid_step_toward(ds, port_far(MAP_PATH_SEGMENT, 0), ARG(0), ARG(1),
                                  (uint8_t)ARG(2), (uint8_t)ARG(3));
        break;
    case 8: opeu_move_clear(port_far(MAP_PATH_SEGMENT, 0), (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 9: r = opeu_find_objective_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 10: r = opeu_objective_side(ds, ARG(0)); break;
    case 11: r = opeu_find_unit_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 12: r = opeu_unit_side(ARG(0)); break;
    case 13: r = opeu_cell_side(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 14: r = opeu_move_supplied(ds); break;
    case 15:
        frame = map_frame_enter();
        opeu_arrival_location(ds, link_mappos, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        map_frame_leave();
        break;
    case 16:
        frame = map_frame_enter();
        opeu_arrival_view(ds, link_mapunit, frame, ARG(0));
        map_frame_leave();
        break;
    case 17:
        frame = map_frame_enter();
        opeu_unit_remove_draw(ds, link_maperase, frame, ARG(0));
        map_frame_leave();
        break;
    case 18:
        frame = map_frame_enter();
        opeu_arrival_prepare(ds, link_maperase, frame, ARG(0));
        map_frame_leave();
        break;
    case 19: opeu_move_clamp(ds); break;
    case 20: r = opeu_cell_marked(ds, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 21: opeu_move_mines(ds, link_movemine, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 22: opeu_move_spot(ds, link_movespot); break;
    case 23: opeu_move_consume(ds, link_movesup); break;
    case 24:
        frame = map_frame_enter();
        r = opeu_move_enemy_adjacent(ds, link_movespot, frame);
        map_frame_leave();
        break;
    case 25: opeu_restore_unit_order(ds, ARG(0)); break;
    case 26: r = opeu_unit_selection_eligible(ds, ARG(0)); break;
    case 27: r = opeu_unit_special(ds, ARG(0)); break;
    case 28: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;
    case 29: map_request_format(ds, args, count); break;
    case 30: opeu_unit_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    default: port_unhandled(module, id, count);
    }
    return r;
}
