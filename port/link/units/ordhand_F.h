/* Shared services of the order handlers of segment 3131 (ORDATTK, ORDDEFND,
 * ORDBRIDG), numbered as tests/validate_order_handlers.py SERVICES:
 *  0 unit_average11(unit)          145c:0562
 *  1 rand_bounded(bound)           09f6:00d4
 *  2 restore_unit_order(unit)      145c:02b4
 *  3 grid_step(&x,&y,dir)          145c:0d3e
 *  4 find_unit_at(x,y)             145c:011c
 *  5 unit_side(unit)               145c:006c
 *  6 unit_value(unit)              0b73:011e
 *  7 move_supplied()               32e6:0288
 *  8 unit_report(unit,text)        1707:07ec
 *  9 unit_selection_eligible(unit) 145c:0fc4
 * 10 request_format(dest,fmt,...)  0480:0b9a
 * 11 confirm_message(text,person)  1707:076c
 * 12 daily_show(id,side,off,seg)   1707:03e4
 * 13 opposite_side(side)           0b73:04c8
 * 14 move_objective_target(&x,&y,side) 32e6:0ae8
 * 15 move_adjacent(x,y)            32e6:0a92
 * 16 move_level_lower(record,v)    32e6:0c3e
 * 17 cell_side(x,y)                145c:0e42
 * 18 objective_quantity(obj,cat,&n) 145c:06c4
 * 19 move_unit(repeat)             32e6:0732
 * 20 side_groups_retarget(side)    190a:06d2
 * 21 move_unit_target(&x,&y)       32e6:0b38
 * 22 unit_name(unit,buffer)        0b73:0188
 * 23 abs16_bits(value)             09cc:029c
 * 24 unit_person(unit)             145c:01c4
 * 25 unit_average10(unit)          145c:052e
 * 26 unit_average9(unit)           145c:04fa
 * 27 unit_order_destination(unit,&x,&y) 145c:02ea
 * 28 map_code(x,y)                 145c:0dc0
 * 29 map_mark_5(x,y)               145c:0da6
 * 30 map_erase(x,y)                15c4:0f7c
 * 31 pending_finish(value)         0a8e:005c
 * 32 wait_units(units)             0a8e:004c (delay_units)
 * 33 sound_cue5()                  0d92:0030
 * Pointer arguments are DS offsets; byte arguments narrow. */
#ifndef PORT_LINK_ORDHAND_F_H
#define PORT_LINK_ORDHAND_F_H

#include <string.h>
#include "units_F.h"

/* 09f6:00d4 rand_bounded: the recovered generator keeps its state in a
 * global; the original state lives at DS:c6f4 (core/random.c). */
extern uint32_t opeu_random_state;
static inline uint16_t lf_rand_bounded(uint8_t *ds, uint16_t bound)
{
    uint16_t r;
    memcpy(&opeu_random_state, ds + 0xc6f4, 4);
    r = opeu_rand_bounded(bound);
    memcpy(ds + 0xc6f4, &opeu_random_state, 4);
    return r;
}

static inline int lf_order_handler_service(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count, uint16_t *result)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0: r = opeu_unit_average11(ds, ARG(0)); break;
    case 1: r = lf_rand_bounded(ds, ARG(0)); break;
    case 2: opeu_restore_unit_order(ds, ARG(0)); break;
    case 3: r = opeu_grid_step(PORT_DS(ARG(0)), PORT_DS(ARG(1)), (uint8_t)ARG(2)); break;
    case 4: r = opeu_find_unit_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 5: r = opeu_unit_side(ARG(0)); break;
    case 6: r = opeu_unit_value(ds, ARG(0)); break;
    case 7: r = opeu_move_supplied(ds); break;
    case 8: opeu_unit_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 9: r = opeu_unit_selection_eligible(ds, ARG(0)); break;
    case 10: r = lf_request_format(ds, args, count); break;
    case 11: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 12:
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1), ARG32(2));
        break;
    case 13: r = opeu_opposite_side((uint8_t)ARG(0)); break;
    case 14: r = opeu_move_objective_target(ds, link_movetgt, ARG(0), ARG(1), (uint8_t)ARG(2)); break;
    case 15:
        sp = port_stack_enter(LF_FRAME);
        r = opeu_move_adjacent(ds, link_movetgt, (uint16_t)(sp - 2u), (uint8_t)ARG(0), (uint8_t)ARG(1));
        port_stack_leave(LF_FRAME);
        break;
    case 16: r = opeu_move_level_lower(ds, ARG(0), (uint8_t)ARG(1)); break;
    case 17: r = opeu_cell_side(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 18: r = opeu_objective_quantity(ds, ARG(0), (uint8_t)ARG(1), PORT_DS(ARG(2))); break;
    case 19:
        sp = port_stack_enter(LF_FRAME);
        opeu_move_unit(ds, link_moveunit, (uint16_t)(sp - 2u), ARG(0));
        port_stack_leave(LF_FRAME);
        break;
    case 20: opeu_side_groups_retarget(ds, (uint8_t)ARG(0)); break;
    case 21:
        sp = port_stack_enter(LF_FRAME);
        r = opeu_move_unit_target(ds, link_movetgt, (uint16_t)(sp - 2u), ARG(0), ARG(1));
        port_stack_leave(LF_FRAME);
        break;
    case 22: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;
    case 23: r = opeu_abs16_bits(ARG(0)); break;
    case 24: r = opeu_unit_person(ds, ARG(0)); break;
    case 25: r = opeu_unit_average10(ds, ARG(0)); break;
    case 26: r = opeu_unit_average9(ds, ARG(0)); break;
    case 27: r = opeu_unit_order_destination(ds, ARG(0), PORT_DS(ARG(1)), PORT_DS(ARG(2))); break;
    case 28: r = opeu_map_code(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 29: opeu_map_mark_5(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 30:
        sp = port_stack_enter(LF_FRAME);
        opeu_map_erase(ds, link_maperase, (uint16_t)(sp - 2u), (uint8_t)ARG(0), (uint8_t)ARG(1));
        port_stack_leave(LF_FRAME);
        break;
    case 31: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;
    case 32: opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0)); break;
    case 33: opeu_sound_cue5(ds, link_soundcue); break;
    default: return 0;
    }
    *result = r;
    return 1;
}

#endif
