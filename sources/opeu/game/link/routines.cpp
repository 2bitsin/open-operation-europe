/* Address-keyed handlers of the native services. The game reaches these
 * library and C-runtime routines by their routine address (far code
 * pointers in the data); each handler converts the argument words
 * of port.hpp's convention to the service's parameters. The handlers of the
 * game's own routines live in their modules. */
#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

auto port_routine_opeu_ui_line(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)game;
  opeu_ui_line(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4));
  return 0;
}
