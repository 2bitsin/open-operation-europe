// The start of a side's turn: the scenario's side initializer runs for a side a player commands.
#include "core/sideinit.hpp"
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

namespace {

// The services opeu_initialize_turn_side calls through its event.
enum SideInitService : uint16_t {
  FIND_SIDE_PERSON = 0, // opeu_find_side_record(side, kind)
  FIND_TEMPLATE    = 1, // opeu_find_person_template(mask, kind)
  SET_CONTEXT      = 2, // opeu_set_turn_context(value)
  RUN_INITIALIZER  = 3, // the scenario's far side initializer (offset, segment)
  FINISH           = 4, // opeu_finish_side_initialization(value)
};

} // namespace

// For a side a player commands: picks its first general and the general's templates of kinds 4 and 5, sets the turn
// context, runs the scenario's initializer and finishes the side's set-up. side_initializing is set meanwhile.
void opeu_initialize_turn_side(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t)) {
  auto& data{ Data(game) };
  data.side_initializing = 1;
  if (data.side_status[data.current_side] == SIDE_PLAYED) {
    data.initial_person = event(game, FIND_SIDE_PERSON, data.current_side, 1, 0, 0);
    uint8_t const mask{ Record<Person>(data, data.initial_person).status };
    data.initial_template4 = event(game, FIND_TEMPLATE, mask, 4, 0, 0);
    data.initial_template5 = event(game, FIND_TEMPLATE, mask, 5, 0, 0);
    event(game, SET_CONTEXT, 15, 0, 0, 0);
    auto const& initializer{ data.side_initializers[std::to_underlying(CurrentScenario(data))] };
    event(game, RUN_INITIALIZER, initializer.offset, initializer.segment, 0, 0);
    event(game, FINISH, 1, 0, 0, 0);
  }
  data.side_initializing = 0;
}

uint16_t svc_sideinit(Game& game, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3) {
  uint16_t r = 0;
  (void)a2;
  (void)a3;
  switch (id) {
  case FIND_SIDE_PERSON: r = opeu_find_side_record(game, (uint8_t)a0, (uint8_t)a1); break;
  case FIND_TEMPLATE:    r = opeu_find_person_template(game, (uint8_t)a0, (uint8_t)a1); break;
  case SET_CONTEXT:      opeu_set_turn_context(game, (uint8_t)a0); break;
  case RUN_INITIALIZER:  r = (uint16_t)port_call(game, (uint32_t)a1 * 16u + a0, NULL, 0); break;
  case FINISH:           opeu_finish_side_initialization(game, (uint8_t)a0); break;
  default:               port_unhandled("sideinit", id, 4);
  }
  return r;
}
