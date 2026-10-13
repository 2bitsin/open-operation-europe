// The defend order: while the regiment stands on its destination, the order's progress falls by a loss that grows
// with the regiment's average value11 and shrinks with the general's leadership and the average value10.
#include "units/orddefnd.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/movetgt.hpp"
#include "types.hpp"
#include "units/order-handler.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/unitpers.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/engineer.hpp"

using namespace opeu::game;

uint8_t opeu_order_defend_loss(Game& game) {
  auto&               data{ Data(game) };
  auto const          person{ opeu_unit_person(game, data.current_unit_ptr) };
  std::uint16_t const average10{ opeu_unit_average10(game, data.current_unit_ptr) };
  auto const          leadership{ static_cast<std::uint16_t>(average10 + Record<Person>(data, person).leadership) / 55 };
  return static_cast<std::uint8_t>(opeu_unit_average11(game, data.current_unit_ptr) / 30 - leadership + 4);
}

// Nonzero (the order step is done) while the regiment holds its destination.
uint16_t opeu_order_defend_run(Game& game) {
  auto& data{ Data(game) };
  if (!StandsOn(data, DestinationCell(game))) return 0;
  auto const loss{ opeu_order_defend_loss(game) };
  opeu_move_level_lower(game, CurrentOrder(data), static_cast<uint8_t>(loss));
  return 1;
}
