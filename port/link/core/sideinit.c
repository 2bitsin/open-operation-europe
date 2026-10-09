/* Link: core/sideinit.c (0d9b:0934 opeu_initialize_turn_side). Services,
 * from the host branch: find side=0 (side, kind), find template=1 (mask,
 * kind), context=2 (value), scenario=3 (offset, segment of the DS:735a
 * opeu_side_initializers entry), finish=4 (value). */
#include "b2_common.h"

uint16_t link_sideinit(uint8_t *ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3)
{
    uint16_t r = 0;
    (void)a2;
    (void)a3;
    switch (id) {
    case 0: r = opeu_find_side_record(ds, (uint8_t)a0, (uint8_t)a1); break;       /* 0b73:02ee */
    case 1: r = opeu_find_person_template(ds, (uint8_t)a0, (uint8_t)a1); break;   /* 0b73:03d2 */
    case 2: opeu_set_turn_context(ds, link_sound, (uint8_t)a0); break;            /* 0d65:00fe */
    case 3: r = (uint16_t)port_call(ds, (uint32_t)a1 * 16u + a0, NULL, 0); break;  /* far scenario briefing */
    case 4: opeu_finish_side_initialization(ds, link_sound, (uint8_t)a0); break;  /* 0d65:0032 */
    default: port_unhandled("sideinit", id, 4);
    }
    return r;
}
