// Data segment DS:b242..b2d3 (146 bytes). Combat messages. DS:b312..b34d is BTLCALC's module-local data and DS:b34e
// onward continues in DGBATTLE.
#include "data/dgcombat.hpp"
#include "data/image.hpp"

namespace opeu::game::detail::dgcombat {

auto InitialCombatText(DataSegment& data) -> void {
  SetText(data.text_b242, "cannot move!");
  SetText(data.text_b24f, "cannot fight!");
  SetText(data.text_b25d, "Defenders");
  SetText(data.text_b267, "\033C%c%s\033C7\012is out of %s!\012They %s");
  SetText(data.battle_siege_stalemate_text,
          "\033M4\033C6\033B0Stalemate\033C7\033M0");
  SetText(data.battle_victory_format,
          "\033M4\033C6\033B0%s Victory!\033C7\033M0");
  SetText(data.battle_stalemate_text,
          "\033M4\033C6\033B0Stalemate\033C7\033M0");
}
} // namespace opeu::game::detail::dgcombat
