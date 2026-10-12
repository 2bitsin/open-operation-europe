// Deployment of both battle sides before the first round: each engaged unit of a side is placed by the player when
// the side is player controlled, otherwise automatically.
#include "types.hpp"
#include "map/dplside.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/dplauto.hpp"
#include "map/dplunit.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t BATTLE_UNITS_PER_SIDE{ 9 };
} // namespace

void opeu_deploy_side(uint8_t* ds, BattleEvent event, uint8_t side) {
  auto& data{ Data(ds) };
  data.battle_ai_side     = side;
  data.battle_active_side = static_cast<uint8_t>(
      Call(ds, event, BattleRoutine::BATTLE_UNIT_SIDE,
           data.battle_units[side * BATTLE_UNITS_PER_SIDE]));
  for (uint8_t index{ 0 }; index < BATTLE_UNITS_PER_SIDE; ++index)
    if (auto const unit{
            data.battle_units[side * BATTLE_UNITS_PER_SIDE + index] }) {
      data.battle_ai_index = index;
      if (Call(ds, event, BattleRoutine::BATTLE_UNIT_CONTROLLED, unit))
        opeu_deploy_unit(ds, event, unit);
      else
        opeu_deploy_auto(ds, event, unit);
    }
}

void opeu_deploy_battle(uint8_t* ds, BattleEvent event) {
  opeu_deploy_side(ds, event, 0);
  opeu_deploy_side(ds, event, 1);
}

// Services of this module: the deployment group's (map/dplauto).
uint16_t svc_dplside(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count) {
  return svc_battle_deploy(ds, service, args, count);
}

auto port_routine_opeu_deploy_battle(std::uint8_t* ds, std::uint16_t const*,
                                     unsigned) -> std::uint32_t {
  opeu_deploy_battle(ds, svc_dplside);
  return 0;
}
