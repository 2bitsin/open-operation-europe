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
// The day's picture of a blown bridge and the link-table address of the routine that ends its show.
constexpr std::uint16_t BRIDGE_DESTROYED_PICTURE{ 0x5b };
constexpr std::uint16_t ORDER_SEGMENT{ 0x3131 };
constexpr std::uint16_t DESTROY_PRESENT{ 0x060c };
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
  auto&             data{ Data(game) };
  ServiceCell const destination;
  auto const        at_x{ destination.XAt() };
  auto const        at_y{ destination.YAt() };
  opeu_unit_order_destination(game, data.current_unit_ptr, game.State().Bytes(at_x, 1).data(), game.State().Bytes(at_y, 1).data());
  if (static_cast<Terrain>(static_cast<std::uint8_t>(opeu_map_code(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y))))) != Terrain::BRIDGE
      || StandsOn(game, at_x, at_y)) {
    opeu_restore_unit_order(game, data.current_unit_ptr);
    return 1;
  }
  if (!opeu_move_adjacent(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)))) return 0;
  auto const loss{ opeu_order_destroy_loss(game) };
  if (opeu_move_level_lower(game, CurrentOrder(data), static_cast<uint8_t>(loss))
      && static_cast<std::uint8_t>(opeu_cell_side(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)))) == NO_SIDE) {
    opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), data.current_unit_ptr, Near(data, data.order_bridge_destroyed));
    opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), BRIDGE_DESTROYED_PICTURE, static_cast<uint8_t>(data.current_side),
                    (static_cast<std::uint32_t>(static_cast<std::uint16_t>(DESTROY_PRESENT)) | static_cast<std::uint32_t>(static_cast<std::uint16_t>(ORDER_SEGMENT)) << 16));
    opeu_map_mark_5(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)));
    opeu_map_erase(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)));
    opeu_restore_unit_order(game, data.current_unit_ptr);
  }
  return 1;
}

auto port_routine_opeu_order_destroy_present(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_order_destroy_present(game);
  return 0;
}

auto port_routine_opeu_order_destroy_run(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_destroy_run(game);
}
