// The staff candidates: their ratings drawn around the base records, and the selected candidate's portrait.
#include "core/staffgen.hpp"
#include "common/events.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/random.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/uiprim.hpp"
#include "data-segment.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace { } // namespace

// A rating drawn around a base: base plus 0..40 capped at 119, then usually 20 less; one time in ten instead capped
// at 99.
uint8_t opeu_staff_jitter_stat(uint8_t v) {
  opeu_add_byte(opeu_rand_bounded(41), 119, &v);
  if (opeu_rand_bounded(10))
    v -= 20;
  else
    v = (uint8_t)opeu_min16(v, 99);
  return v;
}

// Gives the person the staff trait with probability 1 / (3 * (delivery level + 1)), else takes it away.
void opeu_staff_roll_flag(Game& game, uint16_t person) {
  auto& data{ Data(game) };
  auto& record{ Record<Person>(data, person) };
  if (opeu_rand_bounded((data.delivery_level + 1) * 3) == 0)
    record.role |= Bits(PersonRole::STAFF_TRAIT);
  else
    record.role &= ~Bits(PersonRole::STAFF_TRAIT);
}

// Draws the ratings of the current side's candidate `index` again around the candidate's base record; health and its
// companion max_health get the same value.
void opeu_staff_reroll_person(Game& game, uint8_t index) {
  auto&       data{ Data(game) };
  auto&       person{ data.people[data.current_side * PEOPLE_PER_SIDE + index] };
  auto const& base{ data.people_backup[index] };
  person.leadership = opeu_staff_jitter_stat(base.leadership);
  person.charm      = opeu_staff_jitter_stat(base.charm);
  person.bravery    = opeu_staff_jitter_stat(base.bravery);
  person.health = person.max_health = opeu_staff_jitter_stat(base.max_health);
  person.infantry                   = opeu_staff_jitter_stat(base.infantry);
  person.artillery                  = opeu_staff_jitter_stat(base.artillery);
  person.armor                      = opeu_staff_jitter_stat(base.armor);
}

// Shows the selected candidate's portrait if another one is on screen, then draws that candidate's ratings again.
// Returns 0.
uint8_t opeu_staff_reroll_selected(Game& game) {
  auto&          data{ Data(game) };
  uint16_t const person{ data.general_list[data.list_row] };
  if (person != data.shown_portrait) {
    opeu_context_portrait(game, static_cast<const UiHost*>(port_uiprim_host()), 8, 0x100, person);
    data.shown_portrait = person;
  }
  opeu_staff_reroll_person(game, data.list_row);
  return 0;
}
