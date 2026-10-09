/* Argument conversions shared by the C2 link files (request, request-menu,
 * air, mission and advice modules). Conversion only; no game logic. */
#ifndef PORT_LINK_CORE_C2_COMMON_H
#define PORT_LINK_CORE_C2_COMMON_H

#include "link.h"

/* The cdecl variadic text routines of textout (0480:0b9a request_format =
 * sprintf, 0480:0bb8 cursor_print = army_print/context_print) read the
 * pushed argument words at DS `frame` (textout.c REQUEST_PARAMS /
 * PRINT_PARAMS). The words are laid out in the modeled DGROUP stack in push
 * order reversed (destination/format first), as the original call pushed
 * them. */
typedef void (*C2FrameRoutine)(uint8_t *, void *, unsigned short);

static inline void c2_frame_call(uint8_t *ds, C2FrameRoutine routine, const uint16_t *words, unsigned n)
{
    uint16_t bytes = (uint16_t)(2u * n), frame;
    unsigned i;
    port_stack_enter(bytes);
    frame = (uint16_t)(port_sp + 4u);
    for (i = 0; i < n; ++i) {
        ds[(uint16_t)(frame + 2u * i)] = (uint8_t)words[i];
        ds[(uint16_t)(frame + 2u * i + 1u)] = (uint8_t)(words[i] >> 8);
    }
    routine(ds, link_textout_host(), frame);
    port_stack_leave(bytes);
}

/* 0480:0b9a request_format(destination, format, ...) */
static inline void c2_request_format(uint8_t *ds, const uint16_t *words, unsigned n)
{
    c2_frame_call(ds, opeu_request_format, words, n);
}

/* 0480:0bb8 cursor_print/army_print/context_print(format, ...) */
static inline void c2_cursor_print(uint8_t *ds, const uint16_t *words, unsigned n)
{
    c2_frame_call(ds, opeu_cursor_print, words, n);
}

/* A DS region for a callee's `scratch`/`frame` buffer parameter: the base
 * of `bytes` free bytes of the modeled DGROUP stack. Pair with
 * c2_scratch_leave(bytes). */
static inline uint16_t c2_scratch_enter(uint16_t bytes)
{
    port_stack_enter(bytes);
    return (uint16_t)(port_sp + 4u);
}

static inline void c2_scratch_leave(uint16_t bytes)
{
    port_stack_leave(bytes);
}

/* `frame` parameter of the map host branches (mappos/maperase/mapunit/
 * mappick "Host frame convention"): the callee's original BP, or its entry
 * SP minus 2 when it has none; both are the entry SP - 2 of a far call. */
static inline uint16_t c2_frame_enter(void)
{
    return (uint16_t)(port_stack_enter(PORT_FRAME) - 2u);
}

static inline void c2_frame_leave(void)
{
    port_stack_leave(PORT_FRAME);
}

/* Map-cell predicates passed as far code pointers (offset, segment) to
 * 145c:1208 opeu_request_cells: the two predicates of map/cellpred.c. */
typedef uint16_t (*C2CellPredicate)(uint8_t *, uint8_t, uint8_t, uint8_t);

static inline C2CellPredicate c2_cell_predicate(uint16_t off, uint16_t seg)
{
    uint32_t address = (uint32_t)seg * 16u + off;
    if (address == 0x145cu * 16u + 0x1114u) return opeu_request_cell_allowed;
    if (address == 0x145cu * 16u + 0x1152u) return opeu_arrival_cell_allowed;
    port_fatal("request_cells: predicate %04x:%04x is not a cell predicate", seg, off);
    return 0;
}

#endif
