// The final turn action of the selected-turn dispatch. It runs the current order's handler from the order handler
// table (indexed by the order code); an unfinished order counts its hold level down, then its movement level up by
// (99 - average value9) / 35 + 1, and moves with the code's repeat flag. A missing destination clears both order
// slots of the regiment.
#include "units/finalact.hpp"
#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
#include "common/events.hpp"
#include "map/moveunit.hpp"
#include "types.hpp"
#include "units/stats.hpp"

#include "common/game.hpp"
#include "map/movetgt.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/engineer.hpp"
#include "units/ordarriv.hpp"
#include "units/ordattk.hpp"
#include "units/ordbridg.hpp"
#include "units/orddefnd.hpp"
#include "units/order-handler.hpp"
#include "units/orders.hpp"
#include "units/xferunit.hpp"

using namespace opeu::game;

namespace {
// What each order does when its unit reaches it, by the enumerator order_handlers holds for the order's kind: nonzero
// when the order is done with.
constexpr auto ORDER_HANDLERS = DispatchRows<OrderHandler, auto (*)(Game&)->std::uint16_t>({
    { OrderHandler::ARRIVE,           opeu_order_arrive              },
    { OrderHandler::OCCUPY,           opeu_order_occupy_run          },
    { OrderHandler::ATTACK,           opeu_order_attack_run          },
    { OrderHandler::DEFEND,           opeu_order_defend_run          },
    { OrderHandler::LAY_MINES,        opeu_engineer_lay_mines        },
    { OrderHandler::REMOVE_MINES,     opeu_engineer_remove_mines     },
    { OrderHandler::BUILD_BRIDGE,     opeu_engineer_build_bridge     },
    { OrderHandler::DESTROY,          opeu_order_destroy_run         },
    { OrderHandler::SUPPLY_OBJECTIVE, opeu_order_supply_objective    },
    { OrderHandler::SUPPLY_UNIT,      opeu_order_supply_unit         },
    { OrderHandler::REPAIR_OBJECTIVE, opeu_engineer_repair_objective },
    { OrderHandler::REPAIR_UNIT,      opeu_engineer_repair_unit      },
    { OrderHandler::REACH_UNIT,       opeu_order_reach_unit          },
    { OrderHandler::HOLD,             opeu_order_hold                },
});

// The progress an order is given when its hold level runs out.
constexpr std::uint8_t PROGRESS_HELD_OUT{ 200 };
} // namespace

// The order slot and its code are taken before the destination.
void opeu_turn_final_action(Game& game) {
  auto&      data{ Data(game) };
  auto const unit{ data.current_unit_ptr };
  auto const slot{ Near(data, Record<Unit>(data, unit).orders[0]) };
  auto const code{ Record<Order>(data, slot).kind };
  if (!UnitOrderDestination(game, unit).valid) {
    opeu_clear_unit_orders(game, unit, CURRENT_SLOT | SAVED_SLOT);
    return;
  }
  if (Dispatch(ORDER_HANDLERS, data.order_handlers.at(code), game)) return;
  if (opeu_move_level_lower(game, slot, 0)) Record<Order>(data, slot).progress = PROGRESS_HELD_OUT;
  auto const raise{ static_cast<std::uint8_t>(static_cast<std::uint16_t>(99 - opeu_unit_average9(game, data.current_unit_ptr)) / 35 + 1) };
  if (opeu_move_level_raise(game, slot, raise)) opeu_move_unit(game, data.order_moves[code]);
}
