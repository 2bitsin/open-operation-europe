/* units routines reached by address from the battle modules:
 *   units/value.c    0b73:004a category_6 (AL code), 0b73:0056 category_7
 *                    (AL code), 0b73:0178 code_outside_5_to_19 (AL code) -> AX
 *   units/unitname.c 0b73:0188 unit_name (AX buffer, BX unit) -> AX buffer
 *   units/labels.c   0b73:0234 element_name (AX buffer, BX element) -> AX */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_category_6)
{
    (void)ds;
    return opeu_category_6((uint8_t)ARG(0));
}

PORT_ROUTINE(opeu_category_7)
{
    (void)ds;
    return opeu_category_7((uint8_t)ARG(0));
}

PORT_ROUTINE(opeu_code_outside_5_to_19)
{
    (void)ds;
    return opeu_code_outside_5_to_19((uint8_t)ARG(0));
}

PORT_ROUTINE(opeu_unit_name)
{
    return opeu_unit_name(ds, link_unitname, ARG(1), ARG(0));
}

PORT_ROUTINE(opeu_element_name)
{
    return opeu_element_name(ds, ARG(1), ARG(0));
}
