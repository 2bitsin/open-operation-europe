// The start of a side's turn: the scenario's side initializer runs for a side a player commands.
#include "core/sideinit.hpp"
#include "common/dispatch.hpp"
#include "common/events.hpp"
#include "common/scenario.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/people.hpp"
#include "core/scenbrf.hpp"
#include "core/template.hpp"
#include "data-segment.hpp"
#include "io/sound.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {
// The scenarios' briefings, by the enumerator side_initializers holds.
constexpr auto SIDE_INITIALIZERS = DispatchRows<SideInitializer, auto (*)(Game&)->void>({
    { SideInitializer::FRANCE,   opeu_brief_france   },
    { SideInitializer::AFRICA,   opeu_brief_africa   },
    { SideInitializer::KURSK,    opeu_brief_kursk    },
    { SideInitializer::NORMANDY, opeu_brief_normandy },
    { SideInitializer::ARDENNES, opeu_brief_ardennes },
    { SideInitializer::BERLIN,   opeu_brief_berlin   },
});
} // namespace

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
    Dispatch(SIDE_INITIALIZERS, data.side_initializers[std::to_underlying(CurrentScenario(data))], game);
    opeu_finish_side_initialization(game, 1);
  }
  data.side_initializing = 0;
}
