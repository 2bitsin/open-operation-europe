// The daily new-weapon announcement: the element type's panel, then the side's commander announces the type
// ("... Tank." for tanks).
#include "core/daypres.hpp"
#include "common/events.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/people.hpp"
#include "core/template.hpp"
#include "io/soundcue.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t COMMANDER_KIND{ 1 }; // the side's army commander (find_side_record)
constexpr std::uint16_t ANNOUNCER_KIND{ 5 }; // the template that announces a new weapon
constexpr std::uint16_t TEXT_BYTES{ 0x66 };  // the announcement text the format service writes

constexpr std::uint16_t NEW_WEAPON_TANK{ offsetof(DataSegment, new_weapon_tank) };
constexpr std::uint16_t NEW_WEAPON_PERIOD{ offsetof(DataSegment, new_weapon_period) };
constexpr std::uint16_t NEW_WEAPON_FORMAT{ offsetof(DataSegment, new_weapon_format) };

// The speaker: the person template matching the side commander's nation. The commander is found first.
auto Speaker(Game& game) -> std::uint16_t {
  auto&      data{ Data(game) };
  auto const commander{ opeu_find_side_record(game, data.daily_side, COMMANDER_KIND) };
  return opeu_find_person_template(game, Record<Person>(data, commander).status, ANNOUNCER_KIND);
}
} // namespace

// Announces a new weapon type.
void opeu_daily_present(Game& game) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(game) };
  auto const          text{ locals.At(0) };
  opeu_request_wait(game);
  opeu_element_type_panel(game, static_cast<const DialogHost*>(port_dialogs_host()), data.daily_record);
  auto const& type{ Record<ElementType>(data, data.daily_record) };
  opeu_request_format(game, std::array<std::uint16_t, 4>{ text, NEW_WEAPON_FORMAT, type.name, (type.category == Bits(Category::TANKS) ? NEW_WEAPON_TANK : NEW_WEAPON_PERIOD) });
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text, Speaker(game));
}
