/* Routines of io/keyboard.c reached by address from the battle modules:
 *   0541:00ba keyboard_key () -> AL
 *   0541:0436 prompt_value (AX min, DX max, BX text, stack: cancel, items) -> AX
 *   0541:0664 prompt_yes_no (BX text) -> AX
 * The host entries take `scratch`, a 16-byte DS area for the digits and mouse
 * words the original keeps in its frame (keyboard.c host model). */
#include "map/btl_link.h"

#define KEY_SCRATCH 0x10u

PORT_ROUTINE(opeu_keyboard_key)
{
    (void)args; (void)count;
    return opeu_keyboard_key(ds, link_keyboard);
}

PORT_ROUTINE(opeu_prompt_value)
{
    uint16_t scratch = btl_scratch_enter(KEY_SCRATCH), r;
    r = opeu_prompt_value(ds, link_keyboard, ARG(2), ARG(0), ARG(1), (int)(int16_t)ARG(3), ARG(4), scratch);
    btl_scratch_leave(KEY_SCRATCH);
    return r;
}

PORT_ROUTINE(opeu_prompt_yes_no)
{
    uint16_t scratch = btl_scratch_enter(KEY_SCRATCH), r;
    r = (uint16_t)opeu_prompt_yes_no(ds, link_keyboard, ARG(0), scratch);
    btl_scratch_leave(KEY_SCRATCH);
    return r;
}
