/* Data segment DS:8070..838d (798 bytes).
 *
 * Direction keys, turn/order report formats and unit status text.
 * DS:838e..8395 belongs to SELECT.
 *
 * Member names carry their DS offset. Strings keep their exact terminators; z_ members are zero bytes, b_/w_
 * members byte and word tables whose meaning is not yet assigned; p_ members are near pointers (DS offsets), far
 * pointers name their routine or far text.
 */
#include "data/dgturn.hpp"

#pragma pack(1)

/* DS:8070..838d, 798 bytes. Turn and report text. */

struct dg_8070 opeu_turn_text = {
  { '1', '2', '3', '4', '6', '7', '8', '9' },
  "\033C0\033M4%s Regiment %d %s executing orders.\033C7\033M0",
  "    %",
  " N/A",
  "%",
  { 0 },
  "Here is the regiment's present position, sir.\033S",
  "Here's my division's present position, sir.\033S",
  { 0xff, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07,
    0x07, 0xff, 0x08, 0x00, 0x88, 0x00, 0x00, 0x88, 0x00, 0x00, 0x88,
    0x00, 0x00, 0x22, 0x00, 0x00, 0x22, 0x00, 0x00, 0x22, 0x00, 0x18,
    0x00, 0x7a, 0x00, 0x1a, 0x01, 0x40, 0x00, 0x40, 0x00, 0x4e, 0x00 },
  "Hurricane",
  "\033M4   %d.%d.4%d %02d:00 \033C6%3s",
  "\033R\"",
  "\033RM",
  "\033RM",
  "\033RM",
  "\033RV",
  "\033C6\033M4%s",
  "\033M4\033C6",
  "Div %d Cmdr",
  "Commander",
  ": %s",
  "\033M0\033C7\033B1",
  "\033C7\033B0",
  "\? ",
  " ",
  "Is this OK\?",
  "We'll wait %d hours.\n"
  "Is this correct, sir\?",
  "Report from\n"
  "\033C%c%s %s:\033C7\n",
  "Report from\n"
  "\033C%c%s\033C7\n",
  "\033RN\033B1\033C7AP/AT\033C6%3d\033C7/\033C6%2d",
  "\033C7Dfns\033C6%6d",
  "\033C7Speed\033C6%5d",
  "\033C7M.Rng\033C6%5d",
  "\033C7G.Rng\033C6%5d\033C7\033B0",
  "\033RP\033B1  \033C7Number OK\033C6%4d     \033C7Morale  \033C6%3d\n"
  "\033C7Damaged  \033C6%4d     \033C7Training\033C6%3d\n"
  "                  \033C7Fatigue \033C6%3d\033C7\033B0",
  "\033RP\033B1\033C7Number OK\033C6%4d  \033C7Damaged\033C6%4d\033C7\033B0",
  "\033C0\033M4%3d-%3d : ",
  "  %s %s ",
  "\033C7 \033M0",
  "12346789wounded",
  "killed",
  0,
  "General %s's\n"
  "been %s!"
};

#pragma pack()
