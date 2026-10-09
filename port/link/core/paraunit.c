/* Link: core/paraunit.c (0f43:0722..0c22 paratroop units). Services from
 * the host branch comment; the args array also carries outputs back:
 *   0 clear_unit_orders(unit,slots)        1 insert_element(unit,element)
 *   2 unit_quantity(unit,category,count)   args[2] <- count
 *   3 unit_capacities(unit,a,b,c)          args[1..3] <- a,b,c
 *   4 map_code(x,y)                        5 find_objective_at(x,y)
 *   6 objective_side(objective)            7 officer_rating(side,kind)
 *   8 code_outside_5_to_19(code)           9 unit_side(unit)
 *  10 grid_step(x,y,direction)             args[0..1] <- x,y
 *  11 find_unit_at(x,y)                    12 arrival_prepare(unit)
 *  13 unit_casualties(unit,percent,kind,c,d,e)
 *  14 cell_marked(x,y,side)                15 mark_cell(x,y,side)
 *  16 map_erase(x,y)                       17 cell_side(x,y) */
#include "c1_common.h"

uint16_t link_paraunit(uint8_t *ds, uint16_t id, uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0: opeu_clear_unit_orders(ds, ARG(0), (uint8_t)ARG(1)); break;                      /* 145c:020a */
    case 1: opeu_insert_element(ds, ARG(0), ARG(1)); break;                                  /* 145c:03ac */
    case 2: {                                                                                 /* 145c:0596 */
        uint8_t n = (uint8_t)ARG(2);
        r = opeu_unit_quantity(ds, ARG(0), (uint8_t)ARG(1), &n);
        if (count > 2) args[2] = n;
        break;
    }
    case 3: {                                                                                 /* 145c:072c */
        uint16_t a = ARG(1), b = ARG(2), c = ARG(3);
        opeu_unit_capacities(ds, ARG(0), &a, &b, &c);
        if (count > 3) { args[1] = a; args[2] = b; args[3] = c; }
        break;
    }
    case 4: r = opeu_map_code(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;                  /* 145c:0dc0 */
    case 5: r = opeu_find_objective_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;         /* 145c:016a */
    case 6: r = opeu_objective_side(ds, ARG(0)); break;                                      /* 145c:01b8 */
    case 7: r = opeu_officer_rating(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;            /* 0e8e:0000 */
    case 8: r = opeu_code_outside_5_to_19((uint8_t)ARG(0)); break;                           /* 0b73:0178 */
    case 9: r = opeu_unit_side(ARG(0)); break;                                               /* 145c:006c */
    case 10: {                                                                                /* 145c:0d3e */
        uint8_t x = (uint8_t)ARG(0), y = (uint8_t)ARG(1);
        r = opeu_grid_step(&x, &y, (uint8_t)ARG(2));
        if (count > 1) { args[0] = x; args[1] = y; }
        break;
    }
    case 11: r = opeu_find_unit_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;             /* 145c:011c */
    case 12:                                                                                  /* 15c4:0fd0 */
        frame = c1_frame_enter();
        opeu_arrival_prepare(ds, link_maperase, frame, ARG(0));
        c1_frame_leave();
        break;
    case 13:                                                                                  /* 17c4:0b1c */
        frame = c1_frame_enter();
        opeu_unit_casualties(ds, link_casualty, frame, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
        c1_frame_leave();
        break;
    case 14: r = opeu_cell_marked(ds, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break; /* 145c:0eb2 */
    case 15: opeu_mark_cell(ds, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;   /* 145c:0e76 */
    case 16:                                                                                  /* 15c4:0f7c */
        frame = c1_frame_enter();
        opeu_map_erase(ds, link_maperase, frame, (uint8_t)ARG(0), (uint8_t)ARG(1));
        c1_frame_leave();
        break;
    case 17: r = opeu_cell_side(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;                /* 145c:0e42 */
    default: port_unhandled("paraunit", id, count);
    }
    return r;
}
