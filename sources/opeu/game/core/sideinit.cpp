// The start of a side's turn: the scenario's side initializer runs for a side a player commands.
#include "core/sideinit.hpp"
#include "common/events.hpp"
#include "common/scenario.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/people.hpp"
#include "core/template.hpp"
#include "data-segment.hpp"
#include "io/sound.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace { } // namespace

// For a side a player commands: picks its first general and the general's templates of kinds 4 and 5, sets the turn
// context, runs the scenario's initializer and finishes the side's set-up. side_initializing is set meanwhile.
void opeu_initialize_turn_side(Game& game) {
  auto& data{ Data(game) };
  data.side_initializing = 1;
  if (data.side_status[data.current_side] == SIDE_PLAYED) {
    data.initial_person = opeu_find_side_record(game, data.current_side, 1);
    uint8_t const mask{ Record<Person>(data, data.initial_person).status };
    data.initial_template4 = opeu_find_person_template(game, mask, 4);
    data.initial_template5 = opeu_find_person_template(game, mask, 5);
    opeu_set_turn_context(game, 15);
    auto const& initializer{ data.side_initializers[std::to_underlying(CurrentScenario(data))] };
    port_call(game, static_cast<uint32_t>(initializer.segment) * 16u + initializer.offset, nullptr, 0);
    opeu_finish_side_initialization(game, 1);
  }
  data.side_initializing = 0;
}
