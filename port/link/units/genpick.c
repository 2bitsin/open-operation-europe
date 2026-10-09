/* Link: units/genpick.c (06db:0f3e general choice, 06db:121e row draw).
 * Services from the host branch's list:
 *  0 person_set_role(person,value) 0b73:02cc
 *  1 person_set_group(person,value) 0b73:02d8
 *  2 context_portrait(x,y,person) 0a04:046c
 *  3 text_attribute(value) 0480:0838 (opeu_text_paper_set)
 *  4 text_color(color) 0480:082e      5 army_position(x,y) 0480:0808
 *  6 army_print(format,...) 0480:0bb8 7 select_side_color(side) 0b73:04e2
 *  8 table_columns(column,row) 0a8e:0140 -> first column byte. The original
 *    passes the addresses of three frame bytes (first, second, third); they
 *    are placed in a modeled frame here and only `first` is returned.
 *  9 staff_restore_bottom 06db:09c8   10 staff_mark_option(slot) 06db:0a5c
 * 11 staff_frame_option(slot) 06db:0b04
 * 12 show_division_strength(division) 06db:0d64
 * 13 find_vacant_post() 06db:0ea4 -> byte
 * 14 clear_staff_role(side,kind) 06db:0f26
 * 15 clear_division_command(side,group) 06db:0f32 */
#include "units_F.h"

uint16_t link_genpick(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0: opeu_person_set_role(ds, ARG(0), (uint8_t)ARG(1)); break;
    case 1: opeu_person_set_group(ds, ARG(0), (uint8_t)ARG(1)); break;
    case 2: opeu_context_portrait(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 3: opeu_text_paper_set(ds, link_textout_host(), (uint8_t)ARG(0)); break;
    case 4: opeu_text_color(ds, link_textout_host(), ARG(0)); break;
    case 5: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 6: r = lf_cursor_print(ds, args, count); break;
    case 7: opeu_select_side_color(ds, link_sideutil, (uint8_t)ARG(0)); break;
    case 8:
        sp = port_stack_enter(LF_FRAME);
        opeu_table_columns(ds, link_listui, (uint8_t)ARG(0), (uint8_t)ARG(1),
                           (uint16_t)(sp - 2u), (uint16_t)(sp - 4u), (uint16_t)(sp - 6u));
        r = ds[(uint16_t)(sp - 2u)];
        port_stack_leave(LF_FRAME);
        break;
    case 9: opeu_staff_restore_bottom(ds, link_staffpnl); break;
    case 10: opeu_staff_mark_option(ds, link_staffpnl, (int16_t)ARG(0)); break;
    case 11: opeu_staff_frame_option(ds, link_staffpnl, (int16_t)ARG(0)); break;
    case 12: opeu_show_division_strength(ds, link_divstr, (uint8_t)ARG(0)); break;
    case 13: r = opeu_find_vacant_post(ds); break;
    case 14: opeu_clear_staff_role(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 15: opeu_clear_division_command(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    default: port_unhandled("genpick", id, count);
    }
    return r;
}
