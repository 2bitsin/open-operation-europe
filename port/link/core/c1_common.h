/* Argument conversions shared by the C1 link files (turn processing and
 * events). Conversion only; no game logic. */
#ifndef PORT_LINK_CORE_C1_COMMON_H
#define PORT_LINK_CORE_C1_COMMON_H

#include "link.h"

/* The cdecl variadic text routines of textout (0480:0b9a request_format =
 * sprintf, 0480:0bb8 cursor_print = army_print/context_print/turn_label)
 * read the pushed argument words at DS `frame` (textout.c REQUEST_PARAMS /
 * PRINT_PARAMS): first argument word at `frame`, as the original call left
 * them above the return address. */
typedef void (*C1FrameRoutine)(uint8_t *, void *, unsigned short);

static inline void c1_frame_call(uint8_t *ds, C1FrameRoutine routine, const uint16_t *words, unsigned n)
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
static inline void c1_request_format(uint8_t *ds, const uint16_t *words, unsigned n)
{
    c1_frame_call(ds, opeu_request_format, words, n);
}

/* 0480:0bb8 cursor_print/army_print/context_print/turn_label(format, ...) */
static inline void c1_cursor_print(uint8_t *ds, const uint16_t *words, unsigned n)
{
    c1_frame_call(ds, opeu_cursor_print, words, n);
}

/* `frame` parameter of the map host branches (maperase/mapunit/... "Host
 * frame convention"): the callee's original BP, or its entry SP minus 2
 * when it has none; both are the entry SP - 2 of a far call. */
static inline uint16_t c1_frame_enter(void)
{
    return (uint16_t)(port_stack_enter(PORT_FRAME) - 2u);
}

static inline void c1_frame_leave(void)
{
    port_stack_leave(PORT_FRAME);
}

/* A DS region for a callee's `scratch` buffer parameter (e.g. KEYBOARD's
 * 16-byte area): the base of `bytes` free bytes of the modeled DGROUP
 * stack. Pair with c1_scratch_leave(bytes). */
static inline uint16_t c1_scratch_enter(uint16_t bytes)
{
    port_stack_enter(bytes);
    return (uint16_t)(port_sp + 4u);
}

static inline void c1_scratch_leave(uint16_t bytes)
{
    port_stack_leave(bytes);
}

/* Daily presentation: 1707:03e4 daily_show(id, side, far presenter). The
 * presenter is a (offset, segment) pair; DIALOGS calls it back through its
 * present service, i.e. by routine identifier. */
static inline void c1_daily_show(uint8_t *ds, uint16_t id, uint16_t side, uint16_t off, uint16_t seg)
{
    opeu_daily_show(ds, link_dialogs_host(), id, (uint8_t)side, (uint32_t)seg << 16 | off);
}

#endif
