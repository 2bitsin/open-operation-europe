// DS:7f22..7f6c menu labels and DS:7fe0..806f menu descriptor. DS:7f6d..7fdf between them belongs to CTRLTEXT.
#include "data/menutext.hpp"
#include "data/image.hpp"

namespace opeu::game::detail::menutext {

auto InitialMenuText(DataSegment& data) -> void {
  SetText(data.menu_pass_label, " Pass ");
  SetText(data.menu_tact_label, " Tact ");
  SetText(data.menu_city_label, " City ");
  SetText(data.menu_tsfr_label, " Tsfr");
  SetText(data.menu_ofcr_label, " Ofcr ");
  SetText(data.menu_info_label, " Info ");
  SetText(data.menu_orgz_label, " Orgz ");
  SetText(data.menu_rqst_label, " Rqst ");
  SetText(data.menu_air_label, " Air ");
  SetText(data.menu_spfc_label, " SpFc ");
  SetText(data.menu_game_label, " Game ");
  data.control_menu = { { { 0x0002, 0x0000, 0x0006, 0x0700, 0x7f22, 0x0000 },
                          { 0x0009, 0x0000, 0x0006, 0x0700, 0x7f29, 0x0001 },
                          { 0x0010, 0x0000, 0x0006, 0x0700, 0x7f30, 0x0002 },
                          { 0x0017, 0x0000, 0x0006, 0x0700, 0x7f37, 0x0003 },
                          { 0x001e, 0x0000, 0x0006, 0x0700, 0x7f3d, 0x0004 },
                          { 0x0025, 0x0000, 0x0006, 0x0700, 0x7f44, 0x0005 },
                          { 0x002c, 0x0000, 0x0006, 0x0700, 0x7f4b, 0x0006 },
                          { 0x0033, 0x0000, 0x0006, 0x0700, 0x7f52, 0x0007 },
                          { 0x003a, 0x0000, 0x0006, 0x0700, 0x7f59, 0x0008 },
                          { 0x0041, 0x0000, 0x0006, 0x0700, 0x7f5f, 0x0009 },
                          { 0x0048, 0x0000, 0x0006, 0x0700, 0x7f66,
                            0x000a } } };
}
} // namespace opeu::game::detail::menutext
