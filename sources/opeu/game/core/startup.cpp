// Starting a game: ranking the computer sides' generals and giving them their staff roles and divisions, marking
// the units on the map, resetting the turn order and link tables, and seeding the random numbers.
#include "core/startup.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "core/personfl.hpp"
#include "core/random.hpp"
#include "types.hpp"

#include <span>
#include <utility>

#include "common/game.hpp"
#include "core/scensel.hpp"
#include "core/segsetup.hpp"
#include "io/sound.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// The staff roles of a computer side's three ablest generals, best first; the next four lead the divisions.
constexpr StaffRole STAFF_ROLES[]{ StaffRole::ARMY_COMMANDER, StaffRole::OPERATIONS_CHIEF, StaffRole::INTELLIGENCE_CHIEF };

// The steps start-up adds to startup_mode: one for a new game and one for every start.
constexpr uint8_t STARTUP_STEP{ 10 };

// The schedule flags a new game starts with.
constexpr uint8_t NEW_GAME_SCHEDULE{ 7 };

} // namespace

// The sum of the general's eight ability ratings.
uint16_t opeu_person_ability_total(Game const& game, uint16_t person) {
  auto const& general{ Record<Person>(Data(game), person) };
  return general.leadership + general.charm + general.bravery + general.health + general.infantry + general.artillery + general.armor + general.experience;
}

// Stores the indices of the side's present generals in order[], ablest first (a stable sort), and returns how many.
uint8_t opeu_rank_side_people(Game& game, uint8_t side, uint8_t* order) {
  auto&    data{ Data(game) };
  uint16_t totals[PEOPLE_PER_SIDE];
  uint8_t  count{ 0 };
  uint8_t  index{ 0 };
  for (auto const& general : SideGenerals(data, side)) {
    if (general.status != PERSON_VACANT) {
      order[count]  = index;
      totals[count] = opeu_person_ability_total(game, Near(data, general));
      count++;
    }
    index++;
  }
  for (uint8_t d = 0; d < count; d++)
    for (uint8_t f = count - 1; f > d; f--)
      if (totals[f - 1] < totals[f]) {
        std::swap(totals[f - 1], totals[f]);
        std::swap(order[f - 1], order[f]);
      }
  return count;
}

// Gives each computer side's three ablest generals the staff roles and the next four the divisions; marks every
// fielded unit's cell with its side; resets the turn order and the objective and unit links; sets the schedule.
void opeu_assign_side_staff(Game& game) {
  auto& data{ Data(game) };
  for (uint8_t side = 0; side < SIDE_COUNT; side++) {
    if (data.side_control[side] != SIDE_COMPUTER) continue;
    uint8_t ranked[PEOPLE_PER_SIDE];
    opeu_rank_side_people(game, side, ranked);
    auto const generals{ SideGenerals(data, side) };
    uint8_t    rank{ 0 };
    for (auto const role : STAFF_ROLES) opeu_person_set_role(game, Near(data, generals[ranked[rank++]]), std::to_underlying(role));
    for (uint8_t group = 0; group < GROUPS_PER_SIDE; group++) opeu_person_set_group(game, Near(data, generals[ranked[rank++]]), group);
  }
  for (uint8_t i = 0; i < data.units.size(); i++) {
    auto const& unit{ data.units[i] };
    if (unit.flags & Bits(UnitFlag::READY)) {
      auto& cell{ data.map_cells[unit.y * MAP_WIDTH + unit.x] };
      cell &= ~Bits(MapCell::OCCUPANT_MASK);
      cell |= Bits(i < UNITS_PER_SIDE ? MapCell::SIDE_0 : MapCell::SIDE_1);
    }
  }
  for (uint8_t g = 0; g < data.turn_order.size(); g++) data.turn_order[g] = g;
  for (auto& link : data.objective_links) link = 0;
  for (auto& link : data.unit_links) link = 0;
  data.schedule_flags[0] = NEW_GAME_SCHEDULE;
}

// Seeds the random numbers from the clock, prepares the machine and, when a new game is set up, staffs the sides.
void opeu_app_startup_service(Game& game) {
  opeu_rand_seed(port_clock_ticks_low());
  opeu_app_prepare(game);
  auto& data{ Data(game) };
  if (static_cast<uint16_t>(opeu_new_game_setup(game))) {
    opeu_assign_side_staff(game);
    data.startup_mode += STARTUP_STEP;
  }
  data.startup_mode += STARTUP_STEP;
  opeu_vga_clear();
  opeu_finish_side_initialization(game, svc_sound, static_cast<uint8_t>(1));
}
