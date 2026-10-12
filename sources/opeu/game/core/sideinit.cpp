// The start of a side's turn: the scenario's side initializer runs for a side a player commands.
#include "core/sideinit.hpp"
#include "types.hpp"

#include "core/people.hpp"
#include "core/template.hpp"
#include "data-segment.hpp"
#include "glue.hpp"
#include "io/sound.hpp"
#include "port.hpp"
#include "services.hpp"

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
void opeu_initialize_turn_side(uint8_t ds[65536],
                               uint16_t (*event)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t)) {
  auto& data{ Data(ds) };
  data.side_initializing = 1;
  if (data.side_status[data.current_side] == SIDE_PLAYED) {
    data.initial_person = event(ds, FIND_SIDE_PERSON, data.current_side, 1, 0, 0);
    uint8_t const mask{ Record<Person>(data, data.initial_person).status };
    data.initial_template4 = event(ds, FIND_TEMPLATE, mask, 4, 0, 0);
    data.initial_template5 = event(ds, FIND_TEMPLATE, mask, 5, 0, 0);
    event(ds, SET_CONTEXT, 15, 0, 0, 0);
    auto const& initializer{ data.side_initializers[data.scenario & 0x7f] };
    event(ds, RUN_INITIALIZER, initializer.offset, initializer.segment, 0, 0);
    event(ds, FINISH, 1, 0, 0, 0);
  }
  data.side_initializing = 0;
}

uint16_t svc_sideinit(uint8_t* ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3) {
  uint16_t r = 0;
  (void)a2;
  (void)a3;
  switch (id) {
  case FIND_SIDE_PERSON: r = opeu_find_side_record(ds, (uint8_t)a0, (uint8_t)a1); break;
  case FIND_TEMPLATE:    r = opeu_find_person_template(ds, (uint8_t)a0, (uint8_t)a1); break;
  case SET_CONTEXT:      opeu_set_turn_context(ds, svc_sound, (uint8_t)a0); break;
  case RUN_INITIALIZER:  r = (uint16_t)port_call(ds, (uint32_t)a1 * 16u + a0, NULL, 0); break;
  case FINISH:           opeu_finish_side_initialization(ds, svc_sound, (uint8_t)a0); break;
  default:               port_unhandled("sideinit", id, 4);
  }
  return r;
}
