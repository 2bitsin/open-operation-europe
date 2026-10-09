/* Link: units/allocdst.c (20b3:1b4e..1f4d distribution, headquarters
 * choice, finish). Services from the host branch's list:
 *  0 remove_element(unit,element) 145c:0428
 *  1 insert_element(unit,element) 145c:03ac
 *  2 release_side_elements(list,mask) 145c:14d2
 *  3 unit_capacities(unit) 145c:072c -> args[1..3]
 *  4 objective_capacities(objective) 145c:08fc -> args[1..3]
 *  5 supply_exchange_units(unit,other) 20b3:096e
 *  6 supply_exchange_objective(unit,objective) 20b3:09e6
 *  7 unit_name(unit,buffer) 0b73:0188 -> text
 *  8 request_format(buffer,format,text) 0480:0b9a
 *  9 ops_chief_text(text) 20b3:007a
 * 10 list_dialog(dialog,mode,height) 0a8e:0bc0
 * 11 pending_finish(code) 0a8e:005c
 * opeu_alloc_element_values (allocpnl) receives this event but raises
 * nothing. */
#include "units_F.h"

uint16_t link_allocdst(uint8_t *ds, uint16_t id, uint16_t *args, uint16_t count)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0: r = opeu_remove_element(ds, ARG(0), ARG(1)); break;
    case 1: opeu_insert_element(ds, ARG(0), ARG(1)); break;
    case 2: r = opeu_release_side_elements(ds, ARG(0), (uint8_t)ARG(1)); break;
    case 3: opeu_unit_capacities(ds, ARG(0), &args[1], &args[2], &args[3]); break;
    case 4: opeu_objective_capacities(ds, ARG(0), &args[1], &args[2], &args[3]); break;
    case 5:
        /* a, b: the two 32-byte unit-name arrays of the original frame */
        sp = port_stack_enter(LF_FRAME);
        opeu_supply_exchange_units(ds, link_supply, ARG(0), ARG(1),
                                   (uint16_t)(sp - 2u - 0x20u), (uint16_t)(sp - 2u - 0x40u));
        port_stack_leave(LF_FRAME);
        break;
    case 6:
        sp = port_stack_enter(LF_FRAME);
        opeu_supply_exchange_objective(ds, link_supply, ARG(0), ARG(1), (uint16_t)(sp - 2u - 0x20u));
        port_stack_leave(LF_FRAME);
        break;
    case 7: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;
    case 8: r = lf_request_format(ds, args, count); break;
    case 9: opeu_ops_chief_text(ds, link_chief, ARG(0)); break;
    case 10: r = opeu_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 11: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;
    default: port_unhandled("allocdst", id, count);
    }
    return r;
}
