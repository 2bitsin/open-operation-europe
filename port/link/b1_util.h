/* Shared argument-conversion helpers of the B1 link files (core UI, scenario
 * screens, list/menu dialogs). Only stack-model bookkeeping and argument
 * marshalling: no game logic. */
#ifndef PORT_LINK_B1_UTIL_H
#define PORT_LINK_B1_UTIL_H

#include "link.h"

/* Reserve `n` bytes on the modeled DGROUP stack (port.h) and return their DS
 * offset; the callee's own frames are modeled below them. Pair with
 * b1_stack_free(n). Used for the caller-supplied `scratch` areas the host
 * branches document (stack objects of the original frame). */
static inline uint16_t b1_stack_alloc(uint16_t n)
{
    return (uint16_t)(port_stack_enter(n) - n);
}

static inline void b1_stack_free(uint16_t n)
{
    port_stack_leave(n);
}

/* Callee `frame` in the "original BP" convention (MAP*, SCEN* host branches):
 * entry SP minus 2 with PORT_FRAME bytes reserved below. Pair with
 * b1_frame_leave(). */
static inline uint16_t b1_frame_enter(void)
{
    return (uint16_t)(port_stack_enter(PORT_FRAME) - 2);
}

static inline void b1_frame_leave(void)
{
    port_stack_leave(PORT_FRAME);
}

/* Callee `sp` in the "entry SP" convention. */
static inline uint16_t b1_sp_enter(void)
{
    return port_stack_enter(PORT_FRAME);
}

/* Copy the cdecl argument words of a variadic call onto the modeled stack
 * (the original layout after the return address); returns their DS offset. */
static inline uint16_t b1_push_words(const uint16_t *args, unsigned count)
{
    uint16_t at = b1_stack_alloc((uint16_t)(2 * count));
    unsigned i;
    for (i = 0; i < count; i++)
        port_ds_set_word((uint16_t)(at + 2 * i), args[i]);
    return at;
}

/* 0480:0bb8 opeu_army_print / opeu_context_print / opeu_turn_label
 * (host opeu_cursor_print): args = format, then its arguments. */
static inline void b1_cursor_print(uint8_t *ds, const uint16_t *args, unsigned count)
{
    uint16_t frame = b1_push_words(args, count);
    opeu_cursor_print(ds, link_textout_host(), frame);
    b1_stack_free((uint16_t)(2 * count));
}

/* 0480:0b9a opeu_request_format: args = destination, format, arguments. */
static inline void b1_request_format(uint8_t *ds, const uint16_t *args, unsigned count)
{
    uint16_t frame = b1_push_words(args, count);
    opeu_request_format(ds, link_textout_host(), frame);
    b1_stack_free((uint16_t)(2 * count));
}

#endif
