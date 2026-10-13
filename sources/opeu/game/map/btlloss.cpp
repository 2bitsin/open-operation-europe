// After a battle attack: resolve its losses and remove a destroyed target element.
#include "map/btlloss.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcmdr.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

void opeu_battle_loss_check(Game& game, BattleEvent event, uint16_t attacker, uint16_t target) {
  if (attacker)
    Call(game, event, BattleRoutine::BATTLE_RANGED_FIRE, attacker, target);
  else
    Call(game, event, BattleRoutine::BATTLE_MINE_HIT, target);
  if (Data(game).battle_element_destroyed) {
    if (target) opeu_battle_element_destroy(game, event, target);
    if (!static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_COMPUTER_TURN)) && attacker) Call(game, event, BattleRoutine::BATTLE_UPDATE_ELEMENT, attacker);
  }
}

// The services of this module: the commander group's (map/btlcmdr).
uint16_t svc_btlloss(Game& game, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_commander(game, service, args, count);
}

auto port_routine_opeu_battle_loss_check(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_loss_check(game, svc_btlloss, Arg(args, count, 1), Arg(args, count, 0));
  return 0;
}
