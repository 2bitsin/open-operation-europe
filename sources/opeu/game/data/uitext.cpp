/* Data segment DS:7e28..7f21: request labels and commander text.
 * Table entries are near pointers (DS offsets).
 */
#include "data/uitext.hpp"

char     opeu_request_supplies_name[]     = "supplies";
char     opeu_request_battalions_name[]   = "battalions";
char     opeu_request_regiments_name[]    = "regiments";
uint16_t opeu_request_kind_names[3]       = { 0x7e28, 0x7e31, 0x7e3c };
uint8_t  opeu_request_delay_bases[3]      = { 3, 8, 16 };
char     opeu_equipment_delivery_format[] = "%s %u(+%u)\033S";
uint8_t  opeu_request_weights2[6]         = { 100, 80, 60, 50, 100, 100 };
char     opeu_context_name_format[]       = "\033C6\033M4Army Cmdr: %.26s";
char     opeu_context_stats_format[] =
    "\033B1\033C7Leader  \033C6%2d \033C7Inf   \033C6%2d\n"
    "\033C7Charm   \033C6%2d \033C7Arty  \033C6%2d\n"
    "\033C7Brvy    \033C6%2d \033C7Tanks \033C6%2d\n"
    "\033C7Health  \033C%c%2d \033C7Exp   \033C6%2d\033C7\033B0";
char opeu_context_blank_format[] = "\033B1         \033B0";
/* Preserve the separate zero byte before the scenario lookup table. */
uint8_t opeu_context_text_padding = 0;
uint8_t opeu_scenario_contexts[6] = { 1, 2, 3, 4, 5, 6 };
