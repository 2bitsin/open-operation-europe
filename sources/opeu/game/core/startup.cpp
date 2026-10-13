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

#include <algorithm>
#include <array>
#include <functional>
#include <numeric>
#include <ranges>
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

// The sum of the general's eight ability ratings.
auto AbilityTotal(Person const& general) -> uint16_t {
  return static_cast<uint16_t>(general.leadership + general.charm + general.bravery + general.health + general.infantry + general.artillery + general.armor + general.experience);
}

// The indices of the side's present generals, ablest first (equal totals keep their order); the slots after them hold 0.
auto RankedGenerals(DataSegment const& data, uint8_t side) -> std::array<uint8_t, PEOPLE_PER_SIDE> {
  auto const                           generals{ SideGenerals(data, side) };
  std::array<uint8_t, PEOPLE_PER_SIDE> order{};
  auto       present{ std::views::iota(uint8_t{ 0 }, static_cast<uint8_t>(PEOPLE_PER_SIDE)) | std::views::filter([&generals](uint8_t index) { return generals[index].status != PERSON_VACANT; }) };
  auto const end{ std::ranges::copy(present, order.begin()).out };
  std::ranges::stable_sort(order.begin(), end, std::ranges::greater{}, [&generals](uint8_t index) { return AbilityTotal(generals[index]); });
  return order;
}

// Marks the cell of every fielded unit with its side.
void MarkFieldedUnits(DataSegment& data) {
  for (auto const [index, unit] : std::views::zip(std::views::iota(0U), data.units)) {
    if (!(unit.flags & Bits(UnitFlag::READY))) continue;
    auto& cell{ data.map_cells[unit.y * MAP_WIDTH + unit.x] };
    cell &= ~Bits(MapCell::OCCUPANT_MASK);
    cell |= Bits(index < UNITS_PER_SIDE ? MapCell::SIDE_0 : MapCell::SIDE_1);
  }
}

} // namespace

// Gives each computer side's three ablest generals the staff roles and the next four the divisions; marks every
// fielded unit's cell with its side; resets the turn order and the objective and unit links; sets the schedule.
void opeu_assign_side_staff(Game& game) {
  auto& data{ Data(game) };
  for (auto const side : std::views::iota(uint8_t{ 0 }, uint8_t{ SIDE_COUNT })) {
    if (data.side_control[side] != SIDE_COMPUTER) continue;
    auto const ranked{ RankedGenerals(data, side) };
    auto const generals{ SideGenerals(data, side) };
    auto       next{ ranked.begin() };
    for (auto const role : STAFF_ROLES) opeu_person_set_role(game, Near(data, generals[*next++]), std::to_underlying(role));
    for (auto const group : std::views::iota(0U, GROUPS_PER_SIDE)) opeu_person_set_group(game, Near(data, generals[*next++]), static_cast<uint8_t>(group));
  }
  MarkFieldedUnits(data);
  std::iota(data.turn_order.begin(), data.turn_order.end(), uint8_t{ 0 });
  std::ranges::fill(data.objective_links, 0);
  std::ranges::fill(data.unit_links, 0);
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
  opeu_finish_side_initialization(game, 1);
}
