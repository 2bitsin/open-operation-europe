// Highlights an inspected map cell in whichever map is up: the tactical view or the strategic overview.
#include "map/inspect.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/highlite.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace { } // namespace

void opeu_map_inspect(Game& game, uint8_t x, uint8_t y) {
  (Data(game).view_side != NO_SIDE) ? opeu_map_highlight_view(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y))
                                    : opeu_map_highlight_overview(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
}
