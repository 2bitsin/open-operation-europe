/* Link: core/delay.c (1389:012c opeu_choose_turn_delay). Events, from the
 * host branch's comment:
 *   0 text(id) 312c:000c opeu_message_lookup
 *   1 dialog(buffer,24,person,0) 1707:0726 opeu_delay_dialog
 *   2 confirm(word,person) 1707:07aa opeu_delay_confirm. The original passes
 *     the value in AX and the person in BX; DIALOGS takes (person in BX,
 *     hours in AL), so the host routine's argument order is (person, hours).
 * The routine itself is entry 0 of the far-action table DS:opeu_control_actions
 * (data/ctrltext.c); its PORT_ROUTINE is in port/link/routines/delay.c. */
#include "b1_util.h"

uint16_t link_delay(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;
    case 1: r = opeu_delay_dialog(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3)); break;
    case 2: r = opeu_delay_confirm(ds, link_dialogs_host(), ARG(1), (uint8_t)ARG(0)); break;
    default: port_unhandled("delay", id, count);
    }
    return r;
}
