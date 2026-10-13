// The person templates: the first one of a kind that serves the given nations.
#include "core/template.hpp"
#include "common/game.hpp"
#include "data-segment.hpp"
#include "types.hpp"

using namespace opeu::game;

// Near pointer of the first template whose nation bits meet mask and whose staff role is kind. The search does not
// stop at the end of the table: the game only asks for kinds a template exists for.
uint16_t opeu_find_person_template(Game const& game, uint8_t mask, uint8_t kind) {
  auto const& data{ Data(game) };
  for (auto near{ PERSON_TEMPLATES_AT };; near = static_cast<uint16_t>(near + sizeof(Person))) {
    auto const& person{ Record<Person>(data, near) };
    if ((person.status & mask) && person.role == kind) return near;
  }
}
