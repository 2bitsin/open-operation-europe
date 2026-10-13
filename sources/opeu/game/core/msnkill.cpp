// The special-forces assassination of a unit's generals, and the service table every mission routine calls through.
#include "core/msnkill.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/mission.hpp"
#include "core/msnrept.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/personfl.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include <cstddef>
#include <utility>

#include "common/game.hpp"
#include "core/aircargo.hpp"
#include "core/deputy.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/weather.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/mapunit.hpp"
#include "map/objdamag.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/casualty.hpp"
#include "units/unitname.hpp"

using namespace opeu::game;

namespace {

constexpr auto OFFICER{ std::to_underlying(StaffRole::OPERATIONS_CHIEF) };

// Message ids.
constexpr uint16_t ASSASSINATED{ 3 };          // " has been assassinated!"
constexpr uint16_t ASSASSINATION_MISSION{ 6 }; // " assassination mission"

auto Random(Game& game, uint16_t bound) -> uint16_t {
  return port_rand_bounded(game, bound);
}

auto General(Game& game, uint8_t side, StaffRole role) -> uint16_t {
  return opeu_find_side_record(game, side, std::to_underlying(role));
}

auto IsDeputy(Game& game, uint16_t person) -> bool {
  return Record<Person>(Data(game), person).role & Bits(PersonRole::DEPUTY);
}

} // namespace

// Against an enemy unit: the general of its division falls when the unit is the division's selected one; when it is
// the enemy side's selected unit, the intelligence and operations chiefs may fall (1 in 10 each) and the army
// commander be assassinated, a deputy taking his post (1 in 20). Any of these is a success, and every element of the
// unit then loses 30..49 morale.
void opeu_mission_assassinate(Game& game, uint8_t side) {
  ServiceLocals const locals{ 0x50 };
  auto const          text{ locals.At(0) };
  auto const          target{ Data(game).sides[side].order.target };
  auto const          mission_text{ MissionText(game, ASSASSINATION_MISSION) };
  opeu_string_append(game, opeu_mission_target_name(game, side, target, text), mission_text);
  if (opeu_officer_rating(game, side, OFFICER) / 10 > Random(game, 100)) {
    auto       struck{ false };
    auto const enemy{ opeu_opposite_side(side) };
    if (opeu_find_group_selected_unit(game, opeu_unit_group(target)) == target) {
      auto const general{ opeu_find_group_record(game, enemy, opeu_unit_group_index(target)) };
      if (!IsDeputy(game, general)) {
        opeu_mission_person_killed(game, general);
        struck = true;
      }
    }
    if (opeu_find_side_selected_unit(game, enemy) == target) {
      for (auto const role : { StaffRole::INTELLIGENCE_CHIEF, StaffRole::OPERATIONS_CHIEF }) {
        auto const general{ General(game, enemy, role) };
        if (Random(game, 10) == 4 && !IsDeputy(game, general)) {
          opeu_mission_person_killed(game, general);
          struck = true;
        }
      }
      auto const commander{ General(game, enemy, StaffRole::ARMY_COMMANDER) };
      if (Random(game, 20) == 4) {
        Record<Person>(Data(game), commander).role |= Bits(PersonRole::DEPUTY);
        struck                                      = true;
        auto const commander_side{ opeu_person_lacks_side0_flag(game, commander) };
        opeu_set_turn_context(game, static_cast<uint8_t>(0x10));
        opeu_finish_side_initialization(game, static_cast<uint8_t>(0));
        opeu_string_copy(game, text, commander);
        opeu_string_append(game, text, MissionText(game, ASSASSINATED));
        opeu_string_append(game, text, static_cast<uint16_t>(offsetof(DataSegment, mission_bold_off)));
        opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text, General(game, commander_side, StaffRole::INTELLIGENCE_CHIEF));
        opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), FALLEN_PICTURE, static_cast<uint8_t>(commander_side), Presenter::MISSION_DELAY);
      }
    }
    if (struck) {
      auto& data{ Data(game) };
      auto& unit{ Record<Unit>(data, target) };
      if (data.side_status[side] == SIDE_PLAYED) opeu_mission_report_success(game, side, text, unit.x, unit.y);
      opeu_officer_gain(game, side, OFFICER);
      for (auto& element : Elements(data, unit)) opeu_sub_byte(Random(game, 20) + 30, &element.morale);
      return;
    }
  }
  opeu_mission_report_failure(game, side, text);
}
