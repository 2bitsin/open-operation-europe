/* Link: units/genconf.c (06db:137c..163a general assignment screens).
 * Services from the host branch's list:
 *  0 side_backdrop(mode) 0a04:0720 (opeu_backdrop_fill_color)
 *  1 army_position(x,y) 0480:0808     2 army_print(format,...) 0480:0bb8
 *  3 text_color(color) 0480:082e      4 text_attribute(value) 0480:0838 (opeu_text_paper_set)
 *  5 ui_outline(x1,y1,x2,y2,color) 0a04:065a
 *  6 ui_text(text) 0480:0484          7 list_dialog(dialog,mode,height) 0a8e:0bc0
 *  8 commander_list_dialog(dialog,a,b) 0a8e:0818
 *  9 ask_yes_no(text) 0541:0664 (opeu_prompt_yes_no; 16-byte scratch)
 * 10 staff_restore_bottom 06db:09c8   11 staff_restore_left 06db:0a12
 * 12 staff_roll_flag(person) 06db:0c62 13 staff_reroll_person(slot) 06db:0c94
 * 14 show_division_strength(division) 06db:0d64 */
#include "units_F.h"

uint16_t link_genconf(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0: opeu_backdrop_fill_color(ds, link_uiprim_host(), (uint8_t)ARG(0)); break;
    case 1: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 2: r = lf_cursor_print(ds, args, count); break;
    case 3: opeu_text_color(ds, link_textout_host(), ARG(0)); break;
    case 4: opeu_text_paper_set(ds, link_textout_host(), (uint8_t)ARG(0)); break;
    case 5: opeu_ui_outline(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 6: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;
    case 7: r = opeu_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 8: r = opeu_commander_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 9:
        sp = port_stack_enter(LF_FRAME);
        r = (uint16_t)opeu_prompt_yes_no(ds, link_keyboard, ARG(0), (uint16_t)(sp - 0x10u));
        port_stack_leave(LF_FRAME);
        break;
    case 10: opeu_staff_restore_bottom(ds, link_staffpnl); break;
    case 11: opeu_staff_restore_left(ds, link_staffpnl); break;
    case 12: opeu_staff_roll_flag(ds, ARG(0)); break;
    case 13: opeu_staff_reroll_person(ds, (uint8_t)ARG(0)); break;
    case 14: opeu_show_division_strength(ds, link_divstr, (uint8_t)ARG(0)); break;
    default: port_unhandled("genconf", id, count);
    }
    return r;
}
