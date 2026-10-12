// The data modules of the data segment, in address order. DS:2012..27bd (data/mapstate) is still a copied object:
// core/turnord shuffles its own turn order table.
#include "data/image.hpp"
#include "data/ctrltext.hpp"
#include "data/dgair.hpp"
#include "data/dgbattle.hpp"
#include "data/dgbrief.hpp"
#include "data/dgcombat.hpp"
#include "data/dgevents.hpp"
#include "data/dgfont.hpp"
#include "data/dglabels.hpp"
#include "data/dgnames.hpp"
#include "data/dgorders.hpp"
#include "data/dgscen.hpp"
#include "data/dgsetup.hpp"
#include "data/dgstart.hpp"
#include "data/dgsupply.hpp"
#include "data/dgtext.hpp"
#include "data/dgturn.hpp"
#include "data/menutext.hpp"
#include "data/uitext.hpp"

namespace opeu::game::detail::image {

auto InitialDataSegment(DataSegment& data) -> void {
  InitialWorkspace(data);    // DS:0042..2011
  InitialNames(data);        // DS:27be..3e6d
  InitialTextState(data);    // DS:3eba..3eeb
  InitialFont(data);         // DS:3eec..44eb
  InitialSetupText(data);    // DS:44ec..5176
  InitialStartupText(data);  // DS:5184..536b
  InitialLabels(data);       // DS:537c..5bb1
  InitialBriefingText(data); // DS:5bb2..7359
  InitialEventText(data);    // DS:735a..7e27
  InitialUiText(data);       // DS:7e28..7f21
  InitialMenuText(data);     // DS:7f22..7f6c and DS:7fe0..806f
  InitialControlText(data);  // DS:7f6d..7fdf
  InitialTurnText(data);     // DS:8070..838d
  InitialOrderText(data);    // DS:8396..9111
  InitialSupplyText(data);   // DS:9112..9e0f
  InitialAirText(data);      // DS:9e10..ae2d
  InitialCombatText(data);   // DS:b242..b2d3
  InitialBattleText(data);   // DS:b34e..c24f
}
} // namespace opeu::game::detail::image
