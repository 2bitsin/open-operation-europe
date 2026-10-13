// The destroy-bridge order: next to the bridge at its destination, the regiment lowers the order's progress by a skill
// roll; when it runs out and no side holds the cell, the bridge is blown (the cell becomes a river crossing again).
#include "units/ordbridg.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/grid.hpp"
#include "map/maperase.hpp"
#include "map/movetgt.hpp"
#include "map/occupy.hpp"
#include "types.hpp"
#include "units/order-handler.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/unitpers.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/engineer.hpp"

using namespace opeu::game;

namespace {
// The day's picture of a blown bridge.
constexpr std::uint16_t BRIDGE_DESTROYED_PICTURE{ 0x5b };
} // namespace

uint8_t opeu_order_destroy_loss(Game& game) {
  auto&               data{ Data(game) };
  auto const          person{ opeu_unit_person(game, data.current_unit_ptr) };
  std::uint16_t const random{ port_rand_bounded(game, RollBound(data)) };
  std::uint16_t const average10{ opeu_unit_average10(game, data.current_unit_ptr) };
  std::uint16_t const average11{ opeu_unit_average11(game, data.current_unit_ptr) };
  return static_cast<std::uint8_t>(average11 / 19 - static_cast<std::uint16_t>(Record<Person>(data, person).leadership + average10) / 45 + random + 5);
}

void opeu_order_destroy_present(Game& game) {
  opeu_sound_cue5(game);
  opeu_pending_finish(game, static_cast<uint8_t>(SHOW_DONE));
}

uint16_t opeu_order_destroy_run(Game& game) {
  auto& data{ Data(game) };
  auto const [x, y]{ DestinationCell(game) };
  if (static_cast<Terrain>(opeu_map_code(game, x, y)) != Terrain::BRIDGE || StandsOn(data, { x, y })) {
    opeu_restore_unit_order(game, data.current_unit_ptr);
    return 1;
  }
  if (!opeu_move_adjacent(game, x, y)) return 0;
  auto const loss{ opeu_order_destroy_loss(game) };
  if (opeu_move_level_lower(game, CurrentOrder(data), loss) && opeu_cell_side(game, x, y) == NO_SIDE) {
    opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), data.current_unit_ptr, Near(data, data.order_bridge_destroyed));
    opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), BRIDGE_DESTROYED_PICTURE, static_cast<uint8_t>(data.current_side), Presenter::BRIDGE_DESTROYED);
    opeu_map_mark_5(game, x, y);
    opeu_map_erase(game, x, y);
    opeu_restore_unit_order(game, data.current_unit_ptr);
  }
  return 1;
}
