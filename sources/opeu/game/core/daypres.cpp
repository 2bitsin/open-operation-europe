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

#include <cstddef>

using namespace opeu::game;

namespace {

// Services leave the module through event.
enum Service : std::uint16_t {
  WAIT        = 0, // ()
  TYPE_PANEL  = 1, // (element type)
  FORMAT      = 2, // (destination, format, name, suffix)
  SIDE_RECORD = 3, // (side, kind) -> person
  TEMPLATE    = 4, // (status mask, kind) -> person
  MESSAGE     = 5, // (text, person)
};

constexpr std::uint16_t COMMANDER_KIND{ 1 }; // the side's army commander (find_side_record)
constexpr std::uint16_t ANNOUNCER_KIND{ 5 }; // the template that announces a new weapon
constexpr std::uint16_t TEXT_BYTES{ 0x66 };  // the announcement text the format service writes

constexpr std::uint16_t NEW_WEAPON_TANK{ offsetof(DataSegment, new_weapon_tank) };
constexpr std::uint16_t NEW_WEAPON_PERIOD{ offsetof(DataSegment, new_weapon_period) };
constexpr std::uint16_t NEW_WEAPON_FORMAT{ offsetof(DataSegment, new_weapon_format) };

// The speaker: the person template matching the side commander's nation. The commander is found first.
auto Speaker(Game& game, HostEvent event) -> std::uint16_t {
  auto&      data{ Data(game) };
  auto const commander{ Call(game, event, SIDE_RECORD, data.daily_side, COMMANDER_KIND) };
  return Call(game, event, TEMPLATE, Record<Person>(data, commander).status, ANNOUNCER_KIND);
}
} // namespace

// Announces a new weapon type.
void opeu_daily_present(Game& game, HostEvent event) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(game) };
  auto const          text{ locals.At(0) };
  Call(game, event, WAIT);
  Call(game, event, TYPE_PANEL, data.daily_record);
  auto const& type{ Record<ElementType>(data, data.daily_record) };
  Call(game, event, FORMAT, text, NEW_WEAPON_FORMAT, type.name, type.category == Bits(Category::TANKS) ? NEW_WEAPON_TANK : NEW_WEAPON_PERIOD);
  Call(game, event, MESSAGE, text, Speaker(game, event));
}

uint16_t svc_daypres(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case WAIT:        opeu_request_wait(game, svc_soundcue); break;
  case TYPE_PANEL:  opeu_element_type_panel(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case FORMAT:      opeu_request_format(game, { args, count }); break;
  case SIDE_RECORD: r = opeu_find_side_record(game, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1))); break;
  case TEMPLATE:    r = opeu_find_person_template(game, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1))); break;
  case MESSAGE:     opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  default:          port_unhandled("daypres", id, count);
  }
  return r;
}

auto port_routine_opeu_daily_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_daily_present(game, svc_daypres);
  return 0;
}
