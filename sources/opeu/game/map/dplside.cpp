// Deployment of both battle sides before the first round: each engaged unit of a side is placed by the player when
// the side is player controlled, otherwise automatically.
#include "map/dplside.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcalc.hpp"
#include "map/btllist.hpp"
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

void opeu_deploy_side(Game& game, uint8_t side) {
  auto& data{ Data(game) };
  data.battle_ai_side     = side;
  data.battle_active_side = static_cast<uint8_t>(opeu_battle_unit_side(game, data.battle_units[side * BATTLE_UNITS_PER_SIDE]));
  for (uint8_t index{ 0 }; index < BATTLE_UNITS_PER_SIDE; ++index)
    if (auto const unit{ data.battle_units[side * BATTLE_UNITS_PER_SIDE + index] }) {
      data.battle_ai_index = index;
      if (opeu_battle_unit_controlled(game, unit))
        opeu_deploy_unit(game, unit);
      else
        opeu_deploy_auto(game, unit);
    }
}

void opeu_deploy_battle(Game& game) {
  opeu_deploy_side(game, 0);
  opeu_deploy_side(game, 1);
}
