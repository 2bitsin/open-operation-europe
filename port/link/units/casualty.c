/* Link: units/casualty.c (17c4:0b1c unit casualties). Services from the
 * host branch's list:
 *  0 unit_side(unit) 145c:006c             1 unit_group_index(unit) 145c:0048
 *  2 find_side_selected_unit(side) 145c:0ca6
 *  3 find_group_selected_unit(group) 145c:0c46
 *  4 unit_name(unit,buffer) 0b73:0188      5 unit_special(unit) 145c:04de
 *  6 unit_selection_eligible(unit) 145c:0fc4
 *  7 location_blast(x,y,side) 17c4:0744 (frame)
 *  8 unit_resource_percentages(unit,a,b,c) 145c:0a5e (a,b,c DS word offsets)
 *  9 element_name(element,buffer) 0b73:0234
 * 10 rand_bounded(bound) 09f6:00d4 (C runtime state at DS:c6f4)
 * 11 request_format(destination,format,...) 0480:0b9a
 * 12 find_side_record(side,kind) 0b73:02ee
 * 13 general_casualty(record) 17c4:086e (bp)
 * 14 find_group_record(side,group) 0b73:0334
 * 15 unit_side_report(unit,text) 1707:091a
 * 16 arrival_location(x,y,side) 15c4:1408 (frame)
 * 17 isqrt(value) SYSLIB 3FCE              18 remove_element(unit,element) 145c:0428
 * 19 select_preferred_element(unit) 17c4:0ace
 * 20 select_first_unit(group) 17c4:0a78    21 set_record_counter(record,count) 0b73:02de
 * 22 select_first_group(side) 17c4:0a44
 * 23 report_element(element) 1707:0b00 (opeu_group_element_panel)
 * 24 control_message(text,record) 1707:0672
 * 25 unit_capacities(unit,a,b,c) 145c:072c (a,b,c DS word offsets)
 * 26 blast_erase(x,y) 17c4:0860 */
#include "units_F.h"
#include <string.h>

extern uint32_t opeu_random_state;   /* core/random.c host generator state (src/main/include/opeu.h) */

/* The host random generator keeps its state outside ds; the original's C
 * runtime state lives at DS:c6f4 (as genwound.c's own host helper does). */
static uint16_t lf_casualty_rand(uint8_t *ds, uint16_t bound)
{
    uint16_t r;
    memcpy(&opeu_random_state, ds + 0xc6f4, 4);
    r = opeu_rand_bounded(bound);
    memcpy(ds + 0xc6f4, &opeu_random_state, 4);
    return r;
}

uint16_t link_casualty(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t sp, r = 0, a, b, c;
    switch (id) {
    case 0: r = opeu_unit_side(ARG(0)); break;
    case 1: r = opeu_unit_group_index(ARG(0)); break;
    case 2: r = opeu_find_side_selected_unit(ds, (uint8_t)ARG(0)); break;
    case 3: r = opeu_find_group_selected_unit(ds, ARG(0)); break;
    case 4: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;
    case 5: r = opeu_unit_special(ds, ARG(0)); break;
    case 6: r = opeu_unit_selection_eligible(ds, ARG(0)); break;
    case 7:
        sp = port_stack_enter(LF_FRAME);
        opeu_location_blast(ds, link_blast, (uint16_t)(sp - 2u), (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        port_stack_leave(LF_FRAME);
        break;
    case 8:
        a = port_ds_word(ARG(1)); b = port_ds_word(ARG(2)); c = port_ds_word(ARG(3));
        opeu_unit_resource_percentages(ds, ARG(0), &a, &b, &c);
        port_ds_set_word(ARG(1), a); port_ds_set_word(ARG(2), b); port_ds_set_word(ARG(3), c);
        break;
    case 9: r = opeu_element_name(ds, ARG(0), ARG(1)); break;
    case 10: r = lf_casualty_rand(ds, ARG(0)); break;
    case 11: r = lf_request_format(ds, args, count); break;
    case 12: r = opeu_find_side_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 13:
        sp = port_stack_enter(LF_FRAME);
        opeu_general_casualty(ds, link_genwound, (uint16_t)(sp - 2u), ARG(0));
        port_stack_leave(LF_FRAME);
        break;
    case 14: r = opeu_find_group_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 15: opeu_unit_side_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 16:
        sp = port_stack_enter(LF_FRAME);
        opeu_arrival_location(ds, link_mappos, (uint16_t)(sp - 2u), (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        port_stack_leave(LF_FRAME);
        break;
    case 17: r = opeu_isqrt(ARG(0)); break;
    case 18: r = opeu_remove_element(ds, ARG(0), ARG(1)); break;
    case 19: opeu_select_preferred_element(ds, ARG(0)); break;
    case 20: opeu_select_first_unit(ds, ARG(0)); break;
    case 21: opeu_set_record_counter(PORT_DS(ARG(0)), (uint8_t)ARG(1)); break;
    case 22: opeu_select_first_group(ds, (uint8_t)ARG(0)); break;
    case 23: opeu_group_element_panel(ds, link_dialogs_host(), ARG(0)); break;
    case 24: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 25:
        a = port_ds_word(ARG(1)); b = port_ds_word(ARG(2)); c = port_ds_word(ARG(3));
        opeu_unit_capacities(ds, ARG(0), &a, &b, &c);
        port_ds_set_word(ARG(1), a); port_ds_set_word(ARG(2), b); port_ds_set_word(ARG(3), c);
        break;
    case 26: opeu_blast_erase(ds, link_blast, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    default: port_unhandled("casualty", id, count);
    }
    return r;
}
