/* The shared ViewEvent protocol of the MAIN 1fe6 view modules (VWPANEL,
 * VWDIV, VWPEOPLE, VWCITY, VWREGT, VWARMS: the same enum is repeated in each
 * host branch). Words are the callee's arguments (register arguments first,
 * then stack words in push-reversed order), bytes zero-extended; AX is
 * returned. Conversion only. */
#ifndef PORT_LINK_CORE_B2_VIEW_H
#define PORT_LINK_CORE_B2_VIEW_H

#include "b2_common.h"

/* A DS offset as the host pointer the recovered data helpers take. */
#define B2_DS_WORDS(off) ((uint16_t *)(void *)PORT_DS(off))

static inline uint16_t b2_view_service(uint8_t *ds, const char *module, uint16_t id,
                                       const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 1: r = opeu_objective_side(ds, ARG(0)); break;                      /* 145c:01b8 */
    case 2: r = opeu_objective_eligible(ds, ARG(0)); break;                  /* 145c:101a */
    case 3: r = opeu_find_group_selected_unit(ds, ARG(0)); break;            /* 145c:0c46 */
    case 4: r = opeu_unit_selection_eligible(ds, ARG(0)); break;             /* 145c:0fc4 */
    case 5: r = opeu_unit_side(ARG(0)); break;                               /* 145c:006c */
    case 6: opeu_army_title(ds, link_dialogs_host(), ARG(0)); break;         /* 1707:03aa */
    case 7:                                          /* 145c:09fa (objective, &a, &b, &c) */
        opeu_objective_resource_percentages(ds, ARG(0), B2_DS_WORDS(ARG(1)), B2_DS_WORDS(ARG(2)),
                                            B2_DS_WORDS(ARG(3)));
        break;
    case 8: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 9: b2_cursor_print(ds, args, count); break;                         /* 0480:0bb8 */
    case 10: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;   /* 0b73:0188 */
    case 11:                                         /* 145c:072c (unit, &a, &b, &c) */
        opeu_unit_capacities(ds, ARG(0), B2_DS_WORDS(ARG(1)), B2_DS_WORDS(ARG(2)), B2_DS_WORDS(ARG(3)));
        break;
    case 12: r = opeu_resource_percent(ARG(0), ARG(2)); break;   /* 145c:09aa (cur lo,hi, cap lo,hi) */
    case 13:                                         /* 145c:02ea (unit, &x, &y) */
        r = opeu_unit_order_destination(ds, ARG(0), PORT_DS(ARG(1)), PORT_DS(ARG(2)));
        break;
    case 14:                                         /* 0b73:023c (x, y, buffer) */
        r = opeu_location_name(ds, link_labels, (uint8_t)ARG(0), (uint8_t)ARG(1), ARG(2));
        break;
    case 15:                                         /* 15c4:0692 (group, index); frame = BP */
        B2_SP_CALL(PORT_FRAME, opeu_group_show(ds, link_mapunit, (uint16_t)(sp - 2), ARG(0), (uint8_t)ARG(1)));
        break;
    case 16: opeu_backdrop_fill_color(ds, link_uiprim_host(), (uint8_t)ARG(0)); break;   /* 0a04:0720 */
    case 17: r = opeu_find_side_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;     /* 0b73:02ee */
    case 18: r = opeu_find_group_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;    /* 0b73:0334 */
    case 19:                                                                             /* 0bc3:023a */
        opeu_army_heading(ds, link_headings_HeadingEvent, link_headings_HeadingColorEvent, ARG(0));
        break;
    case 20: opeu_division_heading(ds, link_headings_HeadingEvent, ARG(0)); break;       /* 0bc3:02e6 */
    case 21:                                         /* 0bc3:0668 (units, person); sp = entry SP */
        B2_SP_CALL(PORT_FRAME, opeu_division_regiments(ds, link_armypanl, ARG(0), ARG(1), sp));
        break;
    case 22: opeu_division_credits(ds, link_credits); break;                             /* 0bc3:0790 */
    case 23: opeu_screen_reset(ds, link_dialogs_host()); break;                          /* 1707:038e */
    case 24: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;               /* 1389:000e */
    case 25: opeu_control_position(ds, ARG(0), ARG(1)); break;                           /* 15c4:13d8 */
    case 26:                                                                             /* 15c4:1408 */
        B2_SP_CALL(PORT_FRAME, opeu_arrival_location(ds, link_mappos, (uint16_t)(sp - 2),
                                                     (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)));
        break;
    case 27: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;       /* 1707:0672 */
    case 28: opeu_turn_active_begin(ds, link_dialogs_host()); break;                     /* 1707:020c */
    case 29: r = opeu_delay_dialog(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3)); break;
    case 30: opeu_title_panel_begin(ds, link_dialogs_host()); break;                     /* 1707:02e2 */
    case 31: r = opeu_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 32: r = opeu_opposite_side((uint8_t)ARG(0)); break;                             /* 0b73:04c8 */
    case 33:                                         /* 15c4:060e (record, ticks); frame = BP */
        B2_SP_CALL(PORT_FRAME, opeu_unit_flash(ds, link_mapunit, (uint16_t)(sp - 2), ARG(0), (uint8_t)ARG(1)));
        break;
    case 34: b2_request_format(ds, args, count); break;                                  /* 0480:0b9a */
    case 35: opeu_copy_bytes(ds, ARG(0), ARG(1), ARG(2)); break;                         /* 09cc:000a */
    case 36:                                         /* 15c4:064a (unit; DX not read) */
        B2_SP_CALL(PORT_FRAME, opeu_unit_announce(ds, link_mapunit, (uint16_t)(sp - 2), ARG(0)));
        break;
    case 37: r = opeu_unit_special(ds, ARG(0)); break;                                   /* 145c:04de */
    case 38: opeu_element_type_panel(ds, link_dialogs_host(), ARG(0)); break;            /* 1707:0a22 */
    case 39: r = (uint16_t)port_call(ds, (uint32_t)ARG(1) * 16u + ARG(0), NULL, 0); break;  /* far call */
    case 40: r = 0x1fe6; break;          /* code segment of 1fe6 in the identifier numbering */
    default: port_unhandled(module, id, count);
    }
    return r;
}

#endif
