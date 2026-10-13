// After a battle attack: resolve its losses and remove a destroyed target element.
#include "map/btlloss.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcmdr.hpp"
#include "map/btlmines.hpp"
#include "map/btlmove.hpp"
#include "map/btlrfire.hpp"
#include "map/btlturn.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

void opeu_battle_loss_check(Game& game, uint16_t attacker, uint16_t target) {
  if (attacker)
    opeu_battle_ranged_fire(game, attacker, target);
  else
    opeu_battle_mine_hit(game, target);
  if (Data(game).battle_element_destroyed) {
    if (target) opeu_battle_element_destroy(game, target);
    if (!static_cast<uint8_t>(opeu_battle_computer_turn(game)) && attacker) opeu_battle_update_element(game, attacker);
  }
}
