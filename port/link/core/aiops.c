/* Link for the AI modules' boundary pointer (core/aiops1-5.c).
 *
 * The host branches of aiops1..aiops5 call six routines through the global
 * opeu_ai_unrecovered (aiops1.c:143), written when those callees were not
 * yet recovered. All six are now recovered host routines, so the port
 * installs this adapter, which only converts the register-style arguments
 * (AX, DX, and BX passed by address) to the recovered routine's parameters.
 * The mapping follows the boundary's documented ids and the oracle bindings
 * in tests/validate_ai_ops.py:
 *   0 = 145c:154e opeu_release_side_units(ds, list=BX, mask=AX)
 *   1 = 145c:14d2 opeu_release_side_elements(ds, list=BX, mask=AX)
 *   2 = 145c:10a2 opeu_list_due_records(ds, list=BX, mask=AX, kind=DX)
 *   3 = 145c:110e opeu_objective_level(ds, objective=BX)
 *   4 = 145c:101a opeu_objective_eligible(ds, objective=BX)
 *   5 = 1ac2:05a8 opeu_ai_05a8(ds, budget=*BX in/out, amount=AX, which=DX)
 */
#include "link.h"

extern uint16_t (*opeu_ai_unrecovered)(uint8_t *ds, uint16_t id, uint16_t ax, uint16_t dx, uint16_t *bx);

static uint16_t link_ai_unrecovered(uint8_t *ds, uint16_t id, uint16_t ax, uint16_t dx, uint16_t *bx)
{
    switch (id) {
    case 0: return opeu_release_side_units(ds, *bx, (uint8_t)ax);
    case 1: return opeu_release_side_elements(ds, *bx, (uint8_t)ax);
    case 2: return opeu_list_due_records(ds, *bx, (uint8_t)ax, (uint8_t)dx);
    case 3: return opeu_objective_level(ds, *bx);
    case 4: return opeu_objective_eligible(ds, *bx);
    case 5: return opeu_ai_05a8(ds, bx, ax, (uint8_t)dx);
    }
    port_unhandled("ai_unrecovered", id, 3);
    return 0;
}

#if !OPEU_HAVE_PATH_FILL
/* A reference whose unitai.c predates the recovery of the AI path fill
 * 19df:006e reaches it through opeu_path_fill_hook (unitai.c:173). The port
 * does not substitute a fill of its own: it stops with a clear message
 * (build against a reference with the recovered fill, see README). */
extern uint16_t (*opeu_path_fill_hook)(uint8_t *ds, uint8_t sx, uint8_t sy, uint8_t tx, uint8_t ty, uint16_t unit);

static uint16_t link_path_fill_missing(uint8_t *ds, uint8_t sx, uint8_t sy, uint8_t tx, uint8_t ty, uint16_t unit)
{
    port_fatal("unitai: 19df:006e path_fill (%u,%u)->(%u,%u) unit %04x has no host branch in this "
               "reference (src/main/core/unitai.c); not linkable", sx, sy, tx, ty, unit);
    return 0;
}
#endif

/* Installed by main() before the application entry. */
void link_install_ai_hooks(void)
{
    opeu_ai_unrecovered = link_ai_unrecovered;
#if !OPEU_HAVE_PATH_FILL
    opeu_path_fill_hook = link_path_fill_missing;
#endif
}
