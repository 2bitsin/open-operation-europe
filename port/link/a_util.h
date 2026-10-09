/* Argument helpers of the A link files (start-up, input, sound, text). Only
 * stack-model bookkeeping and argument marshalling. */
#ifndef PORT_LINK_A_UTIL_H
#define PORT_LINK_A_UTIL_H

#include "link.h"

/* Push `count` argument words onto the modeled DGROUP stack (the cdecl
 * layout after the return address) and return their DS offset; pair with
 * a_pop_words(count). */
static inline uint16_t a_push_words(const uint16_t *args, unsigned count)
{
    uint16_t at = (uint16_t)(port_stack_enter((uint16_t)(2 * count)) - 2 * count);
    unsigned i;
    for (i = 0; i < count; i++)
        port_ds_set_word((uint16_t)(at + 2 * i), args[i]);
    return at;
}

static inline void a_pop_words(unsigned count)
{
    port_stack_leave((uint16_t)(2 * count));
}

/* 0480:0bb8 army_print / context_print / turn_label (host opeu_cursor_print):
 * words = format, then its arguments. */
static inline void a_cursor_print(uint8_t *ds, const uint16_t *words, unsigned count)
{
    uint16_t frame = a_push_words(words, count);
    opeu_cursor_print(ds, link_textout_host(), frame);
    a_pop_words(count);
}

#endif
