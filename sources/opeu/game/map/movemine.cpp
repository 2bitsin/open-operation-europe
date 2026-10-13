// A unit that moves into a minefield: an engineer unit only reports the mines; any other reports the minefield, shows
// it to a player's side that may see the unit and takes casualties. Either way the cell is marked for the side and the
// unit moves on. Also: element damage capped at the quantity, and the current unit's movement rating.
#include "map/movemine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/sideutil.hpp"
#include "map/cellflag.hpp"
#include "map/maperase.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/casualty.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
enum class MineService : uint16_t {
  IS_ENGINEER   = 0,  // element category code
  UNIT_MESSAGE  = 1,  // unit, text
  UNIT_ELIGIBLE = 2,  // unit
  UNIT_SPOTTED  = 3,  // unit
  DAILY_SHOW    = 4,  // id, side, presenter offset, presenter segment
  CASUALTIES    = 5,  // unit, percent, kind, limit, flags, mode
  MARK_CELL     = 6,  // x, y, side
  UNIT_ERASE    = 7,  // unit
  MORALE        = 8,  // unit: average element morale
  TRAINING      = 9,  // unit: average element training
  EXPERIENCE    = 10, // unit: average element experience
  ISQRT         = 11, // value
};
// The minefield picture and the routine that ends it (opeu_move_mines_finish), by its far address.
constexpr uint16_t MINES_PICTURE{ 0x48 };
constexpr uint16_t MINES_FINISH_OFFSET{ 0x05a0 };
constexpr uint16_t MINES_FINISH_SEGMENT{ 0x32e6 };
} // namespace

void opeu_move_mines(Game& game, HostEvent event, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const current{ data.current_unit_ptr };
  if (Call(game, event, MineService::IS_ENGINEER, data.current_entry)) {
    Call(game, event, MineService::UNIT_MESSAGE, current, Near(data, data.mines_found_text));
  } else {
    Call(game, event, MineService::UNIT_MESSAGE, current, Near(data, data.minefield_text));
    auto const own{ data.current_side };
    auto const enemy{ opeu_opposite_side(data.current_side) };
    if (data.side_status[own] == SIDE_PLAYED && Call(game, event, MineService::UNIT_ELIGIBLE, current))
      Call(game, event, MineService::DAILY_SHOW, MINES_PICTURE, own, MINES_FINISH_OFFSET, MINES_FINISH_SEGMENT);
    else if (data.side_status[enemy] == SIDE_PLAYED && Call(game, event, MineService::UNIT_SPOTTED, current))
      Call(game, event, MineService::DAILY_SHOW, MINES_PICTURE, enemy, MINES_FINISH_OFFSET, MINES_FINISH_SEGMENT);
    Call(game, event, MineService::CASUALTIES, current, 100, 3, 15, 0, 1);
  }
  auto& unit{ Record<Unit>(data, current) };
  Call(game, event, MineService::MARK_CELL, unit.x, unit.y, data.current_side);
  Call(game, event, MineService::UNIT_ERASE, current);
  unit.x     = x;
  unit.y     = y;
  unit.moved = 0;
}

// Caps each element's damage at its quantity.
void opeu_move_clamp(Game& game) {
  auto& data{ Data(game) };
  for (auto& element : Elements(data, Record<Unit>(data, data.current_unit_ptr))) element.damage = opeu_min16(element.quantity, element.damage);
}

// sqrt(max(0, (leader + morale + training) / 3 - experience)), plus the delivery level for a computer side.
uint8_t opeu_move_rating(Game& game, HostEvent event) {
  auto const& data{ Data(game) };
  auto const  current{ data.current_unit_ptr };
  auto const  morale{ Call(game, event, MineService::MORALE, current) };
  auto const  training{ Call(game, event, MineService::TRAINING, current) };
  auto const  experience{ Call(game, event, MineService::EXPERIENCE, current) };
  auto const  leadership{ Record<Person>(data, data.context_group_record).leadership };
  auto        rating{ static_cast<uint16_t>(static_cast<uint16_t>(leadership + morale + training) / 3) };
  auto const  bonus{ data.side_control[data.current_side] == SIDE_COMPUTER ? data.delivery_level : 0 };
  return static_cast<uint8_t>(Call(game, event, MineService::ISQRT, opeu_sub_word(experience, &rating)) + bonus);
}

uint16_t svc_movemine(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  auto const arg{ [&](unsigned i) { return Arg(args, count, i); } };
  auto const byte{ [&](unsigned i) { return static_cast<uint8_t>(Arg(args, count, i)); } };
  switch (static_cast<MineService>(id)) {
  case MineService::IS_ENGINEER:   return opeu_category_6(byte(0));
  case MineService::UNIT_MESSAGE:  opeu_unit_report(game, static_cast<DialogHost const*>(port_dialogs_host()), arg(0), arg(1)); return 0;
  case MineService::UNIT_ELIGIBLE: return opeu_unit_selection_eligible(game, arg(0));
  case MineService::UNIT_SPOTTED:  return opeu_unit_special(game, arg(0));
  case MineService::DAILY_SHOW:    port_daily_show(game, arg(0), arg(1), arg(2), arg(3)); return 0;
  case MineService::CASUALTIES:    opeu_unit_casualties(game, svc_casualty, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5)); return 0;
  case MineService::MARK_CELL:     opeu_mark_cell(game, byte(0), byte(1), byte(2)); return 0;
  case MineService::UNIT_ERASE:    opeu_unit_remove_draw(game, svc_maperase, arg(0)); return 0;
  case MineService::MORALE:        return opeu_unit_average9(game, arg(0));
  case MineService::TRAINING:      return opeu_unit_average10(game, arg(0));
  case MineService::EXPERIENCE:    return opeu_unit_average11(game, arg(0));
  case MineService::ISQRT:         return opeu_isqrt(arg(0));
  default:                         port_unhandled("movemine", id, count);
  }
  return 0;
}
