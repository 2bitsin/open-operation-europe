/* Link: core/coords.c (1389:02a0 opeu_control_coordinates, keyboard/mouse
 * map-coordinate selection). Services, from the host branch's macros:
 *  0 location(x,y,side)   15c4:1408    1 input reset         05a8:01b4
 *  2 save(x,y,w,h,seg,off) VGALIB 2ED8  3 erase(x,y)           15c4:0f7c
 *  4 inspect(x,y)         17c4:00bc    5 cursor(x,y)          17c4:0164
 *  6 objective panel(o)   1fe6:0270    7 unit panel(u)        1fe6:037a
 *  8 army summary()       0bc3:0042    9 mouse show           05a8:003e
 * 10 mouse buttons -> AL  05a8:0170   11 mouse position(&x,&y) 05a8:0086
 * 12 mouse hide           05a8:0062   13 pan(dx,dy,x,side,y)  17c4:0242
 * 14 unit action(unit,g)  1c23:09aa   15 message lookup(id)   312c:000c
 * 16 control message(text,person) 1707:0672  17 keyboard key -> AL 0541:00ba
 * 18 restore(x,y,w,h,mode,seg,off) VGALIB 2674
 * Service 13 packs the _fastcall stack words in memory order (side, then
 * y) after the register words dx, dy, x (tests/validate_coordinates.py). */
#include "b2_common.h"

uint16_t link_coords(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0:
        B2_SP_CALL(PORT_FRAME, opeu_arrival_location(ds, link_mappos, (uint16_t)(sp - 2),
                                                     (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)));
        break;
    case 1: opeu_input_reset(ds, link_mouse); break;
    case 2: opeu_bitmap_save(ARG(0), ARG(1), ARG(2), ARG(3), B2_FAR(ARG(4), ARG(5))); break;
    case 3:                                                  /* maperase frame = entry SP - 2 */
        B2_SP_CALL(PORT_FRAME, opeu_map_erase(ds, link_maperase, (uint16_t)(sp - 2),
                                              (uint8_t)ARG(0), (uint8_t)ARG(1)));
        break;
    case 4: opeu_map_inspect(ds, link_inspect, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 5:                                                  /* cursor frame = BP, 46-byte workspace */
        B2_SP_CALL(PORT_FRAME, opeu_map_cursor(ds, link_cursor, (uint16_t)(sp - 2), ARG(0), ARG(1)));
        break;
    case 6:                                                  /* view sp = entry SP */
        B2_SP_CALL(PORT_FRAME, opeu_objective_panel(ds, link_vwpanel, sp, ARG(0)));
        break;
    case 7:
        B2_SP_CALL(PORT_FRAME, opeu_unit_panel(ds, link_vwpanel, sp, ARG(0)));
        break;
    case 8:                                                  /* armysum scratch = name[32] at BP-0x50 */
        B2_SP_CALL(PORT_FRAME, opeu_army_summary(ds, link_armysum, (uint16_t)(sp - 2 - 0x50)));
        break;
    case 9: opeu_mouse_show(ds, link_mouse); break;
    case 10: r = opeu_mouse_buttons(ds, link_mouse); break;
    case 11: opeu_mouse_position(ds, link_mouse, ARG(0), ARG(1)); break;
    case 12: opeu_mouse_hide(ds, link_mouse); break;
    case 13:
        opeu_viewport_pan(ds, link_viewpan, (int16_t)ARG(0), (int16_t)ARG(1),
                          (uint8_t)ARG(2), (uint8_t)ARG(4), (uint8_t)ARG(3));
        break;
    case 14:                                                 /* ordact sp = entry SP; (unit, unused) */
        B2_SP_CALL(PORT_FRAME, opeu_unit_action(ds, link_ordact, sp, ARG(0), ARG(1)));
        break;
    case 15: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;
    case 16: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 17: r = opeu_keyboard_key(ds, link_keyboard); break;
    case 18: opeu_bitmap_restore(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), B2_FAR(ARG(5), ARG(6))); break;
    default: port_unhandled("coords", id, count);
    }
    return r;
}
