/* DS:7f22..7f6c menu labels and DS:7fe0..806f menu descriptor.
 * DS:7f6d..7fdf between them belongs to CTRLTEXT. Labels are near pointers (DS offsets).
 */
#include "data/menutext.hpp"
#include "data/image.hpp"

using namespace opeu::game;

/* Consumers establish +0 column, +2 pixel row, +4 width in columns,
 * +6 attribute word (low byte is the text colour), +8 label and +10 the
 * selection ID with bit 15 as the save-covered-graphics flag. */

char opeu_menu_pass_label[] = " Pass ";
char opeu_menu_tact_label[] = " Tact ";
char opeu_menu_city_label[] = " City ";
char opeu_menu_tsfr_label[] = " Tsfr";
char opeu_menu_ofcr_label[] = " Ofcr ";
char opeu_menu_info_label[] = " Info ";
char opeu_menu_orgz_label[] = " Orgz ";
char opeu_menu_rqst_label[] = " Rqst ";
char opeu_menu_air_label[]  = " Air ";
char opeu_menu_spfc_label[] = " SpFc ";
char opeu_menu_game_label[] = " Game ";

MenuItem opeu_control_menu[12] = {
  { 2, 0, 6, 0x0700, NearTo(opeu_menu_pass_label, 0x7f22), 0 },
  { 9, 0, 6, 0x0700, NearTo(opeu_menu_tact_label, 0x7f29), 1 },
  { 16, 0, 6, 0x0700, NearTo(opeu_menu_city_label, 0x7f30), 2 },
  { 23, 0, 6, 0x0700, NearTo(opeu_menu_tsfr_label, 0x7f37), 3 },
  { 30, 0, 6, 0x0700, NearTo(opeu_menu_ofcr_label, 0x7f3d), 4 },
  { 37, 0, 6, 0x0700, NearTo(opeu_menu_info_label, 0x7f44), 5 },
  { 44, 0, 6, 0x0700, NearTo(opeu_menu_orgz_label, 0x7f4b), 6 },
  { 51, 0, 6, 0x0700, NearTo(opeu_menu_rqst_label, 0x7f52), 7 },
  { 58, 0, 6, 0x0700, NearTo(opeu_menu_air_label, 0x7f59), 8 },
  { 65, 0, 6, 0x0700, NearTo(opeu_menu_spfc_label, 0x7f5f), 9 },
  { 72, 0, 6, 0x0700, NearTo(opeu_menu_game_label, 0x7f66), 10 },
  { 0, 0, 0, 0, 0, 0 }
};
