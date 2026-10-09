/* Link: core/scenend.c (090d:0008..06ae scenario end, image accessors,
 * shutdown helpers). Service ids, from the host branch's comment, resolved
 * by address (symbols.json; the validator lists the same addresses):
 *   0 set_turn_context(al) 0d65:00fe
 *   1 string_copy_far(dest,offset,segment) 09cc:00f4
 *   2 min16(a,b) 09cc:027e   3 unit_value(unit) 0b73:011e
 *   4 objective_side(obj) 145c:01b8   5 opposite_side(side) 0b73:04c8
 *   6 find_side_record(side,kind) 0b73:02ee
 *   7 request_format(dest,format,month,day,name) 0480:0b9a
 *   8 vga_clear 0000:2aac   9 palette_map_screen 0a04:0898
 *  10 open_resource(name) 0977:000e
 *  11 picture_file(x,y,handle,block) 0a04:01a0
 *  12 picture_file_masked(x,y,handle,block) 0a04:01ce
 *  13 picture_file_at(x,y,handle,low,high) 0a04:0272
 *  14 dos_close 0000:0032   15 army_position 0480:0808
 *  16 army_print(format[,argument]) 0480:0bb8 (host opeu_cursor_print)
 *  17 finish_side_initialization(al) 0d65:0032   18 delay_units(al) 0a8e:004c
 *  19 set_palette(first,count,rgb) 0000:23d5   20 message_lookup 312c:000c
 *  21 input_flush 0480:00a4   22 music_playing 0d65:0088
 *  23 turn_input_a 05a8:0158  24 turn_input_b 0000:4066
 *  25 quit_to_dos 2597:0b8c   26 dos_free(segment) 0000:0106
 *
 * Stack: opeu_app_finish(sp) enters opeu_scenario_end with sp-4, whose
 * buffers lie at BP-1FCh..BP-102h (BP = sp-6). A caller of opeu_app_finish
 * (or opeu_scenario_end) must reserve at least 0x210 bytes below the entry
 * SP it passes; PORT_FRAME (0x100) is not enough. */
#include "b1_util.h"

uint16_t link_scenend(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_set_turn_context(ds, link_sound, (uint8_t)ARG(0)); break;
    case 1: r = opeu_string_copy_far(ds, ARG(0), port_far(ARG(2), 0), ARG(1)); break;
    case 2: r = opeu_min16(ARG(0), ARG(1)); break;
    case 3: r = opeu_unit_value(ds, ARG(0)); break;
    case 4: r = opeu_objective_side(ds, ARG(0)); break;
    case 5: r = opeu_opposite_side((uint8_t)ARG(0)); break;
    case 6: r = opeu_find_side_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 7: b1_request_format(ds, args, count); break;
    case 8: r = (uint16_t)opeu_vga_clear(); break;
    case 9: opeu_palette_map_screen(ds, link_uiprim_host()); break;
    case 10: r = (uint16_t)opeu_open_resource(ds, link_resopen, ARG(0)); break;
    case 11: opeu_picture_file(ds, link_uiprim_host(), ARG(0), ARG(1), (int16_t)ARG(2), ARG(3)); break;
    case 12: opeu_picture_file_masked(ds, link_uiprim_host(), ARG(0), ARG(1), (int16_t)ARG(2), ARG(3)); break;
    case 13:
        opeu_picture_file_at(ds, link_uiprim_host(), ARG(0), ARG(1), (int16_t)ARG(2), (int32_t)ARG32(3));
        break;
    case 14: r = (uint16_t)opeu_dos_close((int16_t)ARG(0)); break;
    case 15: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 16: b1_cursor_print(ds, args, count); break;
    case 17: opeu_finish_side_initialization(ds, link_sound, (uint8_t)ARG(0)); break;
    case 18: opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0)); break;
    case 19: r = (uint16_t)opeu_set_palette((uint8_t)ARG(0), (uint8_t)ARG(1), PORT_DS(ARG(2))); break;
    case 20: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;
    case 21: opeu_input_flush(ds, link_textout_host()); break;
    case 22: r = (uint16_t)opeu_music_playing(ds, link_sound); break;
    case 23: r = (uint16_t)opeu_turn_input_a(ds, link_mouse); break;
    case 24: r = opeu_turn_input_b(); break;
    case 25: opeu_quit_to_dos(ds, link_settings); break;
    case 26: r = (uint16_t)opeu_dos_free(ARG(0)); break;
    default: port_unhandled("scenend", id, count);
    }
    return r;
}
