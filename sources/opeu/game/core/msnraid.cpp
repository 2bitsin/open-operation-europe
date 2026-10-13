// The special-forces raids, sabotage and cuts of communications: each rolls against the operations chief's rating,
// reports the outcome and, on success, damages the target.
#include "core/msnraid.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/mission.hpp"
#include "core/msnrept.hpp"
#include "core/strings.hpp"
#include "data-segment.hpp"
#include "map/objdamag.hpp"
#include "types.hpp"
#include "units/casualty.hpp"
#include "units/unitname.hpp"

#include <cstddef>
#include <utility>

#include "common/game.hpp"
#include "core/msnkill.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

constexpr auto OFFICER{ std::to_underlying(StaffRole::OPERATIONS_CHIEF) };

// Message ids of the cut-communications reports.
constexpr uint16_t CUT_LINES_TEXT{ 0x6a }; // "mission to cut comm of "
constexpr uint16_t LINES_CUT_TAIL{ 0x65 }; // " had their communications cut!"

constexpr auto At(std::size_t offset) -> uint16_t {
  return static_cast<uint16_t>(offset);
}

auto Target(Game& game, uint8_t side) -> uint16_t {
  return Data(game).sides[side].order.target;
}

// Whether the roll succeeds: the officer's rating divided by divisor against 0..99.
auto Succeeds(Game& game, uint8_t side, uint16_t divisor) -> bool {
  return opeu_officer_rating(game, side, OFFICER) / divisor > game.State().RandomBounded(100);
}

} // namespace

void opeu_mission_raid_objective(Game& game, uint8_t side) {
  ServiceLocals const locals{ 0x32 };
  auto const          text{ locals.At(0) };
  auto const          target{ Target(game, side) };
  auto const&         city{ Record<Objective>(Data(game), target) };
  opeu_string_copy(game, text, At(offsetof(DataSegment, raid_attempt_text)));
  opeu_string_append(game, text, target);
  if (!Succeeds(game, side, 4)) {
    opeu_mission_report_failure(game, side, text);
    return;
  }
  if (SidePlayed(Data(game), side))
    opeu_mission_report_success(game, side, text, city.x, city.y);
  else {
    opeu_string_append(game, opeu_string_copy(game, text, target), At(offsetof(DataSegment, raid_attempt_tail)));
    opeu_mission_report_enemy(game, side, text, city.x, city.y);
  }
  opeu_objective_damage(game, target, 30, 3, 1000, 0, 15, 1);
  opeu_officer_gain(game, side, OFFICER);
}

void opeu_mission_raid_unit(Game& game, uint8_t side) {
  ServiceLocals const locals{ 0x3e };
  auto const          name{ locals.At(0x32) };
  auto const          text{ locals.At(0) };
  auto const          target{ Target(game, side) };
  auto const&         unit{ Record<Unit>(Data(game), target) };
  opeu_string_copy(game, text, At(offsetof(DataSegment, raid_unit_text)));
  opeu_string_append(game, text, opeu_unit_name(game, target, name));
  if (!Succeeds(game, side, 4)) {
    opeu_mission_report_failure(game, side, text);
    return;
  }
  if (SidePlayed(Data(game), side))
    opeu_mission_report_success(game, side, text, unit.x, unit.y);
  else {
    opeu_string_append(game, opeu_mission_target_name(game, side, target, text), At(offsetof(DataSegment, raid_unit_tail)));
    opeu_mission_report_enemy(game, side, text, unit.x, unit.y);
  }
  opeu_unit_casualties(game, target, 30, 3, 15, 1, 1);
  opeu_officer_gain(game, side, OFFICER);
}

void opeu_mission_sabotage(Game& game, uint8_t side) {
  ServiceLocals const locals{ 0x32 };
  auto const          text{ locals.At(0) };
  auto const          target{ Target(game, side) };
  auto const&         city{ Record<Objective>(Data(game), target) };
  opeu_string_copy(game, text, At(offsetof(DataSegment, sabotage_text)));
  opeu_string_append(game, text, target);
  if (!Succeeds(game, side, 3)) {
    opeu_mission_report_failure(game, side, text);
    return;
  }
  if (SidePlayed(Data(game), side))
    opeu_mission_report_success(game, side, text, city.x, city.y);
  else {
    opeu_string_append(game, opeu_string_copy(game, text, target), At(offsetof(DataSegment, sabotage_tail)));
    opeu_mission_report_enemy(game, side, text, city.x, city.y);
  }
  opeu_objective_damage(game, target, 10, 2, 5000, 5, 5, 0);
  opeu_officer_gain(game, side, OFFICER);
}

// Cuts a city's communications.
void opeu_mission_objective_3(Game& game, uint8_t side) {
  ServiceLocals const locals{ 0x78 };
  auto const          text{ locals.At(0) };
  auto const          target{ Target(game, side) };
  auto&               city{ Record<Objective>(Data(game), target) };
  opeu_string_copy(game, text, MissionText(game, CUT_LINES_TEXT));
  opeu_string_append(game, text, target);
  opeu_string_append(game, text, At(offsetof(DataSegment, mission3_suffix)));
  if (!Succeeds(game, side, 2)) {
    opeu_mission_report_failure(game, side, text);
    return;
  }
  if (SidePlayed(Data(game), side))
    opeu_mission_report_success(game, side, text, city.x, city.y);
  else {
    opeu_string_copy(game, text, target);
    opeu_string_append(game, text, MissionText(game, LINES_CUT_TAIL));
    opeu_string_append(game, text, At(offsetof(DataSegment, mission3_tail)));
    opeu_mission_report_enemy(game, side, text, city.x, city.y);
  }
  city.flags &= ~Bits(ObjectiveFlag::COMMUNICATIONS);
  opeu_officer_gain(game, side, OFFICER);
}

// Cuts a unit's communications.
void opeu_mission_unit_4(Game& game, uint8_t side) {
  ServiceLocals const locals{ 0x6e };
  auto const          text{ locals.At(0x1e) };
  auto const          name{ locals.At(0) };
  auto const          target{ Target(game, side) };
  auto&               data{ Data(game) };
  auto&               unit{ Record<Unit>(data, target) };
  opeu_string_copy(game, text, MissionText(game, CUT_LINES_TEXT));
  opeu_string_append(game, text, opeu_unit_name(game, target, name));
  if (!Succeeds(game, side, 2)) {
    opeu_mission_report_failure(game, side, text);
    return;
  }
  if (SidePlayed(Data(game), side))
    opeu_mission_report_success(game, side, text, unit.x, unit.y);
  else {
    auto const tail{ MissionText(game, LINES_CUT_TAIL) };
    opeu_string_append(game, opeu_mission_target_name(game, side, target, text), tail);
    opeu_mission_report_enemy(game, side, text, unit.x, unit.y);
  }
  // The views redraw.
  data.view_side = data.overview_side  = 0xff;
  unit.flags                          &= ~Bits(UnitFlag::COMMUNICATIONS);
  opeu_officer_gain(game, side, OFFICER);
}
