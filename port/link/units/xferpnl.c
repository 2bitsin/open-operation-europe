/* Link: units/xferpnl.c (31f4:0006..05a7 supply-transfer panel), and the
 * service numbering it shares with XFERSTEP and XFERUNIT (xfer_F.h), from
 * tests/validate_supply_transfer.py SERVICES (segment:offset, registers):
 *  1 mouse_show                 05a8:003e
 *  2 delay_ticks(t)             0a8e:000c
 *  3 mouse_position -> args[0..1] = x, y (DOS: opeu_mouse_position(&x,&y))
 *                               05a8:0086
 *  4 mouse_right -> AX          05a8:012e
 *  5 mouse_left -> AX           05a8:0104
 *  6 mouse_hide                 05a8:0062
 *  7 keyboard_key -> AL         0541:00ba
 *  8 bitmap_restore(x,y,w,h,mode,segment,offset)  VGALIB 2674
 *  9 army_position(x,y)         0480:0808
 * 10 army_print(format,...)     0480:0bb8
 * 11 vga_glyph(x,y,w,h,colour,near glyph)  VGALIB 2A81
 * 12 army_title(text)           1707:03aa
 * 13 ems_map_dialog             06ac:0056
 * 14 ui_box(x,y,w,h,colour)     VGALIB 2AD2
 * 15 sound_cue14                0d92:0078
 * 16 sound_cue16                0d92:0088
 * 17 music_command7             0d65:01b0
 * 18 pending_finish(value)      0a8e:005c
 * 19 daily_context(side)        1389:000e
 * 20 confirm_ok(person) -> AX   1707:079e
 * 22 input_reset                05a8:01b4
 * 23 ui_highlight(x,y,w,h,colour)  VGALIB 2B0E
 * 24 unit_report(unit,text)     1707:07ec
 * 25 unit_selection_eligible(unit)  145c:0fc4
 * 26 daily_show(id,side,offset,segment)  1707:03e4 (far presenter)
 * 27 unit_capacities(unit,a,b,c)          145c:072c, a..c DS word offsets
 * 28 objective_capacities(objective,a,b,c) 145c:08fc, a..c DS word offsets
 * 29 side_resource_percentages(side,a,b,c,units_only) 145c:0b3c
 * 30 unit_index(unit)           145c:0034
 * 31 category_7(code)           0b73:0056
 * 32 unit_group(unit)           145c:0086
 * 33 unit_name(unit,buffer)     0b73:0188
 * 34 unit_average10(unit)       145c:052e
 * 35 unit_average11(unit)       145c:0562
 * 36 move_objective_target(x_address,y_address,side)  32e6:0ae8
 * 37 move_unit_target(x_address,y_address)            32e6:0b38 (frame)
 * 38 move_adjacent(x,y)                               32e6:0a92 (frame)
 * 39 move_level_lower(record,value)                   32e6:0c3e
 * 40 restore_unit_order(unit)   145c:02b4 */
#include "xfer_F.h"

/* Host pointer of a DS word the callee writes (output arguments). */
static uint16_t *lf_xferpnl_word(uint16_t off)
{
    return (uint16_t *)(void *)PORT_DS(off);
}

uint16_t lf_xfer_service(uint8_t *ds, const char *module, uint16_t id, uint16_t *args, uint16_t count)
{
    uint16_t sp, at, r = 0;
    switch (id) {
    case 1: opeu_mouse_show(ds, link_mouse); break;
    case 2: opeu_delay_ticks(ds, link_waitkey, ARG(0)); break;
    case 3:
        sp = port_stack_enter(4);
        at = (uint16_t)(sp - 4u);
        opeu_mouse_position(ds, link_mouse, at, (uint16_t)(at + 2u));
        args[0] = port_ds_word(at);
        args[1] = port_ds_word((uint16_t)(at + 2u));
        port_stack_leave(4);
        break;
    case 4: r = (uint16_t)opeu_mouse_right(ds, link_mouse); break;
    case 5: r = (uint16_t)opeu_mouse_left(ds, link_mouse); break;
    case 6: opeu_mouse_hide(ds, link_mouse); break;
    case 7: r = opeu_keyboard_key(ds, link_keyboard); break;
    case 8:
        r = (uint16_t)opeu_bitmap_restore(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
        break;
    case 9: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 10: r = lf_cursor_print(ds, args, count); break;
    case 11: r = (uint16_t)opeu_vga_glyph(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), PORT_DS(ARG(5))); break;
    case 12: opeu_army_title(ds, link_dialogs_host(), ARG(0)); break;
    case 13: opeu_ems_map_dialog(ds, link_emsmap); break;
    case 14: r = (uint16_t)opeu_ui_box(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 15: opeu_sound_cue14(ds, link_soundcue); break;
    case 16: opeu_sound_cue16(ds, link_soundcue); break;
    case 17: opeu_music_command7(ds, link_sound); break;
    case 18: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;
    case 19: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;
    case 20: r = opeu_confirm_ok(ds, link_dialogs_host(), ARG(0)); break;
    case 22: opeu_input_reset(ds, link_mouse); break;
    case 23: r = (uint16_t)opeu_ui_highlight(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 24: opeu_unit_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 25: r = opeu_unit_selection_eligible(ds, ARG(0)); break;
    case 26:
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1),
                        (unsigned int)((uint32_t)ARG(3) << 16 | ARG(2)));
        break;
    case 27:
        opeu_unit_capacities(ds, ARG(0), lf_xferpnl_word(ARG(1)), lf_xferpnl_word(ARG(2)), lf_xferpnl_word(ARG(3)));
        break;
    case 28:
        opeu_objective_capacities(ds, ARG(0), lf_xferpnl_word(ARG(1)), lf_xferpnl_word(ARG(2)), lf_xferpnl_word(ARG(3)));
        break;
    case 29:
        opeu_side_resource_percentages(ds, (uint8_t)ARG(0), lf_xferpnl_word(ARG(1)), lf_xferpnl_word(ARG(2)),
                                       lf_xferpnl_word(ARG(3)), ARG(4));
        break;
    case 30: r = opeu_unit_index(ARG(0)); break;
    case 31: r = opeu_category_7((uint8_t)ARG(0)); break;
    case 32: r = opeu_unit_group(ARG(0)); break;
    case 33: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;
    case 34: r = opeu_unit_average10(ds, ARG(0)); break;
    case 35: r = opeu_unit_average11(ds, ARG(0)); break;
    case 36: r = opeu_move_objective_target(ds, link_movetgt, ARG(0), ARG(1), (uint8_t)ARG(2)); break;
    case 37:
        sp = port_stack_enter(LF_FRAME);
        r = opeu_move_unit_target(ds, link_movetgt, (uint16_t)(sp - 2u), ARG(0), ARG(1));
        port_stack_leave(LF_FRAME);
        break;
    case 38:
        sp = port_stack_enter(LF_FRAME);
        r = opeu_move_adjacent(ds, link_movetgt, (uint16_t)(sp - 2u), (uint8_t)ARG(0), (uint8_t)ARG(1));
        port_stack_leave(LF_FRAME);
        break;
    case 39: r = opeu_move_level_lower(ds, ARG(0), (uint8_t)ARG(1)); break;
    case 40: opeu_restore_unit_order(ds, ARG(0)); break;
    default: port_unhandled(module, id, count);
    }
    return r;
}

uint16_t link_xferpnl(uint8_t *ds, uint16_t id, uint16_t *args, uint16_t count)
{
    return lf_xfer_service(ds, "xferpnl", id, args, count);
}
