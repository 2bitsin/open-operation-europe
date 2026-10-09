/* Link: core/specfrc.c (24d5:0a82..0c2a the Special Forces command). Its
 * host branch uses the service numbering of SPECOPS ("Host service ids are
 * listed in specops.c"): 2 confirm_message, 4 army_print, 8 delay_dialog,
 * 9 army_title, 12 army_position, 13 daily_context, 14 far action
 * (segment, offset) from DS:adb2, 18 unit_name, 20 find_person_template.
 * The callees are the same, so the service goes to SPECOPS's adapter. */
#include "c1_common.h"

uint16_t link_specfrc(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return link_specops(ds, id, args, count);
}
