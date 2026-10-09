/* Argument conversions shared by the B2 link files (command loop, map
 * inspection, views, panels and supply tables). Conversion only; no game
 * logic. */
#ifndef PORT_LINK_CORE_B2_COMMON_H
#define PORT_LINK_CORE_B2_COMMON_H

#include "link.h"

/* The cdecl variadic text routines of textout (0480:0b9a request_format =
 * sprintf; 0480:0bb8 cursor_print = army_print/context_print/turn_label)
 * read the pushed argument words at DS `frame` (textout.c REQUEST_PARAMS /
 * PRINT_PARAMS): destination/format first, then the converted words. The
 * words are placed in the modeled DGROUP stack. */
typedef void (*B2FrameRoutine)(uint8_t *, void *, unsigned short);

static inline void b2_frame_call(uint8_t *ds, B2FrameRoutine routine, const uint16_t *words, unsigned n)
{
    uint16_t bytes = (uint16_t)(2u * n), frame;
    unsigned i;
    frame = (uint16_t)(port_stack_enter(bytes) - bytes);
    for (i = 0; i < n; ++i) {
        ds[(uint16_t)(frame + 2u * i)] = (uint8_t)words[i];
        ds[(uint16_t)(frame + 2u * i + 1u)] = (uint8_t)(words[i] >> 8);
    }
    routine(ds, link_textout_host(), frame);
    port_stack_leave(bytes);
}

/* 0480:0b9a request_format(destination, format, ...) */
static inline void b2_request_format(uint8_t *ds, const uint16_t *words, unsigned n)
{
    b2_frame_call(ds, opeu_request_format, words, n);
}

/* 0480:0bb8 cursor_print = army_print = context_print = turn_label(format, ...) */
static inline void b2_cursor_print(uint8_t *ds, const uint16_t *words, unsigned n)
{
    b2_frame_call(ds, opeu_cursor_print, words, n);
}

/* Host pointer of a far (segment, offset) pair as the event passes it. */
#define B2_FAR(seg, off) port_far((uint16_t)(seg), (uint16_t)(off))

/* Call a routine with an `sp` parameter documented as "the DS offset of SP
 * at entry (far return address)": reserve `bytes` below it. */
#define B2_SP_CALL(bytes, call) do { \
        uint16_t sp = port_stack_enter(bytes); \
        call; \
        port_stack_leave(bytes); \
    } while (0)

/* The TableEvent protocol of the 0c45 row-draw callbacks (tblgroup.c host
 * comment; tblpers.c "as TBLGROUP"):
 *  0 column layout(column, row, out0, out1, out2) 0a8e:0140 (three DS bytes)
 *  1 position(x, y) 0480:0808   2 sprintf(dest, format, ...) 0480:0b9a
 *  3 text color 0480:082e       4 side color 0b73:04e2
 *  5 ui text 0480:0484          6 print(format, ...) 0480:0bb8
 *  7 put range(s, start, count) 0480:0bd0
 *  8 request text(id) -> DS offset 312c:000c message_lookup */
static inline uint16_t b2_table_service(uint8_t *ds, const char *module, uint16_t id,
                                        const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_table_columns(ds, link_listui, (uint8_t)ARG(0), (uint8_t)ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 1: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 2: b2_request_format(ds, args, count); break;
    case 3: opeu_text_color(ds, link_textout_host(), ARG(0)); break;
    case 4: opeu_select_side_color(ds, link_sideutil, (uint8_t)ARG(0)); break;
    case 5: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;
    case 6: b2_cursor_print(ds, args, count); break;
    case 7: opeu_text_put_range(ds, link_textout_host(), ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 8: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;
    default: port_unhandled(module, id, count);
    }
    return r;
}

/* Frame of a 0c45 row-draw callback reached by identifier (LISTUI event
 * 19: (column, first, row, last, flags), bytes zero-extended): `scratch`
 * holds the column outputs at +0..2 and text buffers from +4 (at most
 * 0x68 bytes), placed below the callback's BP in a modeled frame. */
#define B2_TABLE_SCRATCH(sp) ((uint16_t)((sp) - 2 - 0x70))

#endif
