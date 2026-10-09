/* Link: core/aircargo.c (0f43:0f46..149e, cargo drops and the hourly air
 * operations). Services, from the host branch's event list (aircargo.c:20):
 *   0 air_report(name,hours)        0f43:01da   1 air_damage_roll(x,y,range) 0f43:028a
 *   2 air_destroyed_message(name)   0f43:0362   3 air_sprite_draw(x,y,side)  0f43:00e0
 *   4 air_sprite_restore(x,y)       0f43:01ac   5 air_strike_due()           0f43:0390
 *   6 air_strike_event()            0f43:0596   7 para_drop_due()            0f43:0ce8
 *   8 para_drop_event()             0f43:0dbc   9 find_side_record(side,kind) 0b73:02ee
 *  10 find_person_template(mask,kind) 0b73:03d2 11 opposite_side(side)       0b73:04c8
 *  12 side_for_status(side)         0b73:04d0  13 request_message(text,person) 1707:0672
 *  14 sound cue                     0d92:0028  15 request_format(dest,format,...) 0480:0b9a
 *  16 map_center(x,y,side)          15c4:13f6  17 view_pixel(x addr,y addr)  15c4:055c
 *  18 daily_show(id,side,off,seg)   1707:03e4  19 music_command7()           0d65:01b0
 *  20 delay_ticks(ticks)            0a8e:000c  21 find_unit_at(x,y)          145c:011c
 *  22 find_objective_at(x,y)        145c:016a  23 unit_side(unit)            145c:006c
 *  24 objective_side(objective)     145c:01b8  25 unit_capacities(unit,a,b,c) 145c:072c
 *  26 unit_name(unit,buffer)        0b73:0188  27 officer_gain(side,kind)    0e8e:002c
 * Id 26 is unit_name in both branches (the DOS FORMAT_UNIT macro calls
 * opeu_unit_name(u,n); request_format is id 15), as validate_air_cargo.py
 * hooks it. The sprite routines take the load segment: the port keeps the
 * original paragraph numbering, so it is 0. */
#include "c2_air.h"

uint16_t link_aircargo(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0, bp;
    switch (id) {
    case 0:
        bp = c2_air_bp_enter();
        r = opeu_air_report(ds, link_airshow, bp, ARG(0), (uint8_t)ARG(1));
        c2_air_leave();
        break;
    case 1: r = opeu_air_damage_roll(ds, link_airstrk, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 2:
        bp = c2_air_bp_enter();
        opeu_air_destroyed_message(ds, link_airstrk, bp, ARG(0));
        c2_air_leave();
        break;
    case 3:
        bp = c2_air_bp_enter();
        opeu_air_sprite_draw(ds, link_airshow, 0, bp, ARG(0), ARG(1), (uint8_t)ARG(2));
        c2_air_leave();
        break;
    case 4: opeu_air_sprite_restore(ds, link_airshow, 0, ARG(0), ARG(1)); break;
    case 5: r = opeu_air_strike_due(ds, link_airstrk); break;
    case 6:
        bp = c2_air_bp_enter();
        opeu_air_strike_event(ds, link_airstrk, bp);
        c2_air_leave();
        break;
    case 7: r = opeu_para_drop_due(ds, link_paradrop); break;
    case 8:
        bp = c2_air_bp_enter();
        opeu_para_drop_event(ds, link_paradrop, 0, bp);
        c2_air_leave();
        break;
    case 9: r = opeu_find_side_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 10: r = opeu_find_person_template(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 11: r = opeu_opposite_side((uint8_t)ARG(0)); break;
    case 12: r = opeu_side_for_status(ds, (uint8_t)ARG(0)); break;
    case 13: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 14: opeu_sound_cue4(ds, link_soundcue); break;
    case 15: c2_request_format(ds, args, count); break;
    case 16:
        bp = c2_frame_enter();
        opeu_map_center(ds, link_mappos, bp, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c2_frame_leave();
        break;
    case 17: opeu_view_pixel(ds, ARG(0), ARG(1)); break;
    case 18:
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1), (uint32_t)ARG(3) << 16 | ARG(2));
        break;
    case 19: opeu_music_command7(ds, link_sound); break;
    case 20: opeu_delay_ticks(ds, link_waitkey, ARG(0)); break;
    case 21: r = opeu_find_unit_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 22: r = opeu_find_objective_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 23: r = opeu_unit_side(ARG(0)); break;
    case 24: r = opeu_objective_side(ds, ARG(0)); break;
    case 25:
        opeu_unit_capacities(ds, ARG(0), (uint16_t *)(ds + ARG(1)), (uint16_t *)(ds + ARG(2)),
                             (uint16_t *)(ds + ARG(3)));
        break;
    case 26: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;
    case 27: opeu_officer_gain(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    default: port_unhandled("aircargo", id, count);
    }
    return r;
}
