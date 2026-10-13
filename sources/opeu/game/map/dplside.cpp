// Deployment of both battle sides before the first round: each engaged unit of a side is placed by the player when
// the side is player controlled, otherwise automatically.
#include "map/dplside.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/dplauto.hpp"
#include "map/dplunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t BATTLE_UNITS_PER_SIDE{ 9 };
} // namespace

void opeu_deploy_side(Game& game, BattleEvent event, uint8_t side) {
  auto& data{ Data(game) };
  data.battle_ai_side     = side;
  data.battle_active_side = static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_UNIT_SIDE, data.battle_units[side * BATTLE_UNITS_PER_SIDE]));
  for (uint8_t index{ 0 }; index < BATTLE_UNITS_PER_SIDE; ++index)
    if (auto const unit{ data.battle_units[side * BATTLE_UNITS_PER_SIDE + index] }) {
      data.battle_ai_index = index;
      if (Call(game, event, BattleRoutine::BATTLE_UNIT_CONTROLLED, unit))
        opeu_deploy_unit(game, event, unit);
      else
        opeu_deploy_auto(game, event, unit);
    }
}

void opeu_deploy_battle(Game& game, BattleEvent event) {
  opeu_deploy_side(game, event, 0);
  opeu_deploy_side(game, event, 1);
}

// Services of this module: the deployment group's (map/dplauto).
uint16_t svc_dplside(Game& game, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_deploy(game, service, args, count);
}

auto port_routine_opeu_deploy_battle(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_deploy_battle(game, svc_dplside);
  return 0;
}
