/* Link: core/sideutil.c (0b73:04c8..0501 side helpers). The only service is
 * opeu_select_side_color's text colour: event(ds, colour) -> 0480:082e
 * opeu_text_color(colour). */
#include "b2_common.h"

void link_sideutil(uint8_t *ds, uint16_t color)
{
    opeu_text_color(ds, link_textout_host(), color);
}
