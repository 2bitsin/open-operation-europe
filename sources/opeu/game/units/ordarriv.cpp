// The move, pursue and rest order handlers and the hold delay. A handler returns 1 when the current regiment's order
// step is finished. The destination and target services write the cell into the frame bytes x and y.
#include "units/ordarriv.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "core/dialogs.hpp"
#include "map/movetgt.hpp"
#include "types.hpp"
#include "units/order-handler.hpp"
#include "units/orders.hpp"
#include "units/unitpers.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/engineer.hpp"

using namespace opeu::game;

namespace {
// The bound of the draw an element's experience is held against when a resting regiment resumes its saved order.
constexpr std::uint16_t RESUME_DRAW{ 15 };
} // namespace

// The move order: at the destination, the regiment reports its arrival and its saved order resumes.
uint16_t opeu_order_arrive(Game& game) {
  auto& data{ Data(game) };
  if (!StandsOn(data, DestinationCell(game))) return 0;
  opeu_unit_report(game, data.current_unit_ptr, Near(data, data.arrived_text));
  opeu_restore_unit_order(game, data.current_unit_ptr);
  return 1;
}

// The pursue order: finished beside a valid target regiment; a lost target restores the saved order.
uint16_t opeu_order_reach_unit(Game& game) {
  if (auto const [unit, cell]{ opeu_move_unit_target(game) }; unit) return opeu_move_adjacent(game, cell.x, cell.y) ? 1 : 0;
  opeu_restore_unit_order(game, Data(game).current_unit_ptr);
  return 1;
}

// The hold delay: (99 - the general's charm) / 30 + 1, in unsigned 16 bits.
uint8_t opeu_order_hold_delay(Game& game) {
  auto&      data{ Data(game) };
  auto const person{ opeu_unit_person(game, data.current_unit_ptr) };
  return static_cast<std::uint8_t>(static_cast<std::uint16_t>(99 - Record<Person>(data, person).charm) / 30 + 1);
}

// The rest order: hold at the destination. Once the hold runs out and an order is saved, the first element whose
// experience is below a draw of 0..14 resumes that order.
uint16_t opeu_order_hold(Game& game) {
  auto& data{ Data(game) };
  if (!StandsOn(data, DestinationCell(game))) return 0;
  // The delay is taken before the current regiment is read again.
  auto const delay{ opeu_order_hold_delay(game) };
  if (opeu_move_level_lower(game, CurrentOrder(data), static_cast<uint8_t>(delay)) && CurrentUnit(data).orders[1].kind != ORDER_UNUSED)
    for (auto const& element : Elements(data, CurrentUnit(data)))
      // The draw comes before the element is read.
      if (auto const draw{ game.State().RandomBounded(RESUME_DRAW) }; element.experience < draw) {
        opeu_restore_unit_order(game, data.current_unit_ptr);
        break;
      }
  return 1;
}
