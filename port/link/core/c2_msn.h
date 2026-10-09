/* The special-mission service table shared by the 0e8e modules (msnrept,
 * msnkill, msnraid, msnrecon, msntick). The report helpers of msnrept are
 * called with the caller's own event, so every mission callback answers the
 * whole table (ids from the host-branch comments of msnrept.c, msnkill.c,
 * msnraid.c, msnrecon.c and msntick.c; callees from their DOS branches):
 *   0 rand_bounded(bound) 09f6:00d4 (state at DS:c6f4)
 *   1 request_text(id) = message_lookup 312c:000c
 *   2 request_format(dest,format,colour,name,text) 0480:0b9a
 *   3 control_message(text,person) 1707:0672
 *   4 daily_show(id,side,offset,segment) 1707:03e4
 *   5 location_flash(x,y,side) 15c4:087e
 *   6 unit_name(unit,buffer) 0b73:0188
 *   7 objective_damage(objective,chance,base,supply,endurance,spread,heavy) 17c4:10cc
 *   8 unit_casualties(unit,percent,kind,c,d,e) 17c4:0b1c
 *   9 set_turn_context(v) 0d65:00fe      10 finish_side_initialization(v) 0d65:0032
 *   11 initialize_deputy(person) 0b73:048a 12 pending_finish(v) 0a8e:005c
 *   13 delay_units(v) 0a8e:004c           14 weather_update 0f43:000e
 *   15 turn_prepare 1707:00c8             16 air_operations 0f43:1452
 *   17 mission handler from the DS:7a54 far table (offset,segment,side)
 * Conversion only. */
#ifndef PORT_LINK_CORE_C2_MSN_H
#define PORT_LINK_CORE_C2_MSN_H

#include <string.h>
#include "c2_common.h"

/* 09f6:00d4 rand_bounded: the recovered generator keeps its state in a
 * global; the original state lives at DS:c6f4 (core/random.c). */
extern uint32_t opeu_random_state;
static inline uint16_t c2_rand_bounded(uint8_t *ds, uint16_t bound)
{
    uint16_t r;
    memcpy(&opeu_random_state, ds + 0xc6f4, 4);
    r = opeu_rand_bounded(bound);
    memcpy(ds + 0xc6f4, &opeu_random_state, 4);
    return r;
}

static inline uint16_t c2_msn_service(const char *module, uint8_t *ds, uint16_t id,
                                      const uint16_t *args, uint16_t count)
{
    uint16_t r = 0, frame, sp, words[1];
    switch (id) {
    case 0: r = c2_rand_bounded(ds, ARG(0)); break;                                /* 09f6:00d4 */
    case 1: r = opeu_message_lookup(ds, link_msglook, (short)ARG(0)); break;       /* 312c:000c */
    case 2: c2_request_format(ds, args, count); break;                            /* 0480:0b9a */
    case 3: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break; /* 1707:0672 */
    case 4:                                                           /* 1707:03e4 */
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1), ARG32(2));
        break;
    case 5:                                                           /* 15c4:087e */
        frame = c2_frame_enter();
        opeu_location_flash(ds, link_mapunit, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c2_frame_leave();
        break;
    case 6: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;         /* 0b73:0188 */
    case 7:                                                           /* 17c4:10cc */
        frame = c2_frame_enter();
        opeu_objective_damage(ds, link_objdamag, frame, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6));
        c2_frame_leave();
        break;
    case 8:                                                           /* 17c4:0b1c */
        frame = c2_frame_enter();
        opeu_unit_casualties(ds, link_casualty, frame, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
        c2_frame_leave();
        break;
    case 9: opeu_set_turn_context(ds, link_sound, (uint8_t)ARG(0)); break;         /* 0d65:00fe */
    case 10: opeu_finish_side_initialization(ds, link_sound, (uint8_t)ARG(0)); break; /* 0d65:0032 */
    case 11: opeu_initialize_deputy(ds, ARG(0)); break;                            /* 0b73:048a */
    case 12: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;        /* 0a8e:005c */
    case 13: opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0)); break;           /* 0a8e:004c */
    case 14: opeu_weather_update(ds); break;                                       /* 0f43:000e */
    case 15: opeu_turn_prepare(ds, link_dialogs_host()); break;                    /* 1707:00c8 */
    case 16:                                                          /* 0f43:1452, entry SP */
        sp = port_stack_enter(PORT_FRAME);
        opeu_air_operations(ds, link_aircargo, sp);
        port_stack_leave(PORT_FRAME);
        break;
    case 17:                                                          /* far handler (off,seg)(side) */
        words[0] = ARG(2);
        port_call(ds, (uint32_t)ARG(1) * 16u + ARG(0), words, 1);
        break;
    default: port_unhandled(module, id, count);
    }
    return r;
}

#endif
