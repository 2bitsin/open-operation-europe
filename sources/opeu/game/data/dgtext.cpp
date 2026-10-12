/* Data segment DS:3eba..3eeb (50 bytes): the initialised variables of the text output (core/textout) and the
 * keyboard's idle hook (io/keyboard), after the data of the file and VGA libraries; DGFONT continues at DS:3eec.
 * z_ members are zero bytes whose meaning is not yet assigned.
 */
#include "data/dgtext.hpp"

#pragma pack(1)

/* DS:3eba..3eeb, 50 bytes. */

struct dg_3eba opeu_text_vars = { 0, { 0 },  0,      0, 0,    0x0050,
                                  0, 0x00c8, 0x0010, 0, 0x00, 0x07,
                                  0, 0,      0,      0, 0,    0x07,
                                  0, 0,      0,      0 };

#pragma pack()
