/* Shared service numbering of the supply-transfer modules XFERPNL, XFERSTEP
 * and XFERUNIT (31f4): the ids of tests/validate_supply_transfer.py
 * (SERVICES), one routine per id. Defined in port/link/units/xferpnl.c. */
#ifndef PORT_LINK_UNITS_XFER_F_H
#define PORT_LINK_UNITS_XFER_F_H

#include "units_F.h"

uint16_t lf_xfer_service(uint8_t *ds, const char *module, uint16_t id, uint16_t *args, uint16_t count);

#endif
