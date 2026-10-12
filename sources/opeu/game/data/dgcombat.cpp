/* Data segment DS:b242..b2d3 (146 bytes).
 *
 * Combat messages. DS:b312..b34d is BTLCALC's module-local data and
 * DS:b34e onward continues in DGBATTLE.
 *
 * Member names carry their DS offset. Strings keep their exact terminators; z_ members are zero bytes, b_/w_
 * members byte and word tables whose meaning is not yet assigned; p_ members are near pointers (DS offsets), far
 * pointers name their routine or far text.
 */
#include "data/dgcombat.hpp"

#pragma pack(1)

/* DS:b242..b2d3, 146 bytes. Combat messages; DS:b2d4..b2fd are BTLREADY's
 * literals and DS:b2fe..b311 ("TURN %d") is BTLSETUP's. */

struct dg_b242 opeu_combat_text = { "cannot move!",
                                    "cannot fight!",
                                    "Defenders",
                                    "\033C%c%s\033C7\n"
                                    "is out of %s!\n"
                                    "They %s",
                                    "\033M4\033C6\033B0Stalemate\033C7\033M0",
                                    "\033M4\033C6\033B0%s Victory!\033C7\033M0",
                                    "\033M4\033C6\033B0Stalemate\033C7\033M0" };

#pragma pack()
