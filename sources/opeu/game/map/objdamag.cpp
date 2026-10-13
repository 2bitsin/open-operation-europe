// The damage report of a bombed objective: the location flashes, each kind of equipment held there loses some of its
// quantity (more on a hit, infantry many times more under heavy bombing) and part of the other losses goes back
// to stock, the three supply stocks lose some, the garrison strength drops, and each loss is reported.
#include "map/objdamag.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "map/blast.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {
enum class DamageService : uint16_t {
  LOCATION_BLAST = 0, // x, y, side
  FORMAT         = 1, // buffer, format, words...
  SIDE_REPORT    = 2, // objective, text
  ELEMENT_PANEL  = 3, // item
  APPEND         = 4, // destination, source
  BLAST_ERASE    = 5, // x, y
  TITLE_PANEL    = 6, //
  OBJECTIVE_SIDE = 7, // objective
  OPPOSITE_SIDE  = 8, // side
};
// The DS reserves of the report buffers the services format, and their offsets in them; the loss line's holds the
// never-written slot an unknown supply kind reads.
constexpr uint16_t LINE_RESERVE{ 0x68 };
constexpr uint16_t LINE_TEXT_AT{ 0 };
constexpr uint16_t STALE_SLOT_AT{ 0x66 };
constexpr uint16_t REPORT_RESERVE{ 0x8e };
constexpr uint16_t LOSS_TEXT_AT{ 0 };
constexpr uint16_t REPORT_AT{ 0x20 };

// The side letter of the report's colour code: '5' for side 1, '6' for side 0.
[[nodiscard]] constexpr auto SideColour(uint8_t side) -> uint16_t {
  return side ? '5' : '6';
}
} // namespace

// Removes amount..2 * amount - 1 of one of the objective's supply stocks (kind 0..2) and reports it.
void opeu_objective_loss_line(uint8_t* ds, HostEvent event, uint16_t objective, uint8_t kind, uint16_t amount) {
  auto&               data{ Data(ds) };
  auto&               record{ Record<Objective>(data, objective) };
  ServiceLocals const locals{ LINE_RESERVE };
  auto const          text{ locals.At(LINE_TEXT_AT) };
  // An unknown kind takes the stale word in its slot as the stock's address.
  auto const stock{ kind < 3 ? Near(data, record.resources[kind]) : Word(ds, locals.At(STALE_SLOT_AT)) };
  auto const before{ Word(ds, stock) };
  uint16_t   value{ before };
  opeu_sub_word(RandomBelow(ds, amount) + amount, &value);
  SetWord(ds, stock, value);
  auto const after{ Word(ds, stock) };
  auto const colour{ SideColour(static_cast<uint8_t>(Call(ds, event, DamageService::OBJECTIVE_SIDE, objective))) };
  Call(ds, event, DamageService::FORMAT, text, Near(data, data.damage_supply_format), colour, objective, data.supply_names[kind], static_cast<uint16_t>(before - after), data.supply_names[kind],
       after);
  Call(ds, event, DamageService::SIDE_REPORT, objective, text);
}

// bp: this routine's frame. chance: the percent chance of a hit; base: the damage of a hit (none: no equipment loss);
// spread: the random part of it; heavy: infantry loses 6 to 15 times as much; supply and endurance: the supply and
// strength losses.
void opeu_objective_damage(uint8_t* ds, HostEvent event, uint16_t objective, uint16_t chance, uint16_t base, uint16_t supply, uint16_t endurance, uint16_t spread, uint16_t heavy) {
  auto&               data{ Data(ds) };
  auto&               record{ Record<Objective>(data, objective) };
  ServiceLocals const locals{ REPORT_RESERVE };
  auto const          report{ locals.At(REPORT_AT) };
  auto const          loss_text{ locals.At(LOSS_TEXT_AT) };
  auto const          side{ static_cast<uint8_t>(Call(ds, event, DamageService::OBJECTIVE_SIDE, objective)) };
  auto const          enemy{ static_cast<uint8_t>(Call(ds, event, DamageService::OPPOSITE_SIDE, side)) };
  Call(ds, event, DamageService::LOCATION_BLAST, record.x, record.y, enemy);
  if (base) {
    Call(ds, event, DamageService::FORMAT, report, Near(data, data.damage_title));
    Call(ds, event, DamageService::FORMAT, report, Near(data, data.damage_heading), SideColour(side), objective);
    Call(ds, event, DamageService::SIDE_REPORT, objective, report);
    for (auto& item : record.items) {
      if (item.quantity == 0) continue;
      auto const& type{ data.element_types[item.type] };
      bool const  infantry{ type.category == Bits(Category::INFANTRY) };
      uint16_t    lost{ RandomBelow(ds, 100) >= chance ? RandomBelow(ds, spread >> 1) : static_cast<uint16_t>(RandomBelow(ds, spread) + base) };
      if (heavy && infantry) lost = static_cast<uint16_t>(lost * (RandomBelow(ds, 10) + 6));
      lost          = opeu_min16(item.quantity, lost);
      item.quantity = static_cast<uint16_t>(item.quantity - lost);
      Call(ds, event, DamageService::FORMAT, report, Near(data, data.damage_element_format), SideColour(side), data.side_names[side], type.name);
      Call(ds, event, DamageService::ELEMENT_PANEL, Near(data, item));
      if (lost == 0) {
        Call(ds, event, DamageService::APPEND, report, Near(data, data.damage_none));
        Call(ds, event, DamageService::SIDE_REPORT, objective, report);
        continue;
      }
      Call(ds, event, DamageService::FORMAT, loss_text, lost == 1 ? Near(data, data.damage_loss_one) : Near(data, data.damage_loss_many), lost);
      Call(ds, event, DamageService::APPEND, report, loss_text);
      Call(ds, event, DamageService::SIDE_REPORT, objective, report);
      if (!infantry) {
        uint16_t stock{ item.extra };
        opeu_add_word(RandomBelow(ds, lost / 3), data.request_capacities[type.category], &stock);
        item.extra = stock;
      }
    }
  }
  if (supply) {
    opeu_objective_loss_line(ds, event, objective, 1, supply);
    opeu_objective_loss_line(ds, event, objective, 0, supply);
    opeu_objective_loss_line(ds, event, objective, 2, supply);
  }
  if (endurance) {
    auto const before{ record.strength };
    opeu_sub_byte(RandomBelow(ds, endurance) + endurance, &record.strength);
    Call(ds, event, DamageService::FORMAT, report, Near(data, data.damage_endurance_format), SideColour(side), objective, record.strength, before - record.strength);
    Call(ds, event, DamageService::SIDE_REPORT, objective, report);
  }
  Call(ds, event, DamageService::BLAST_ERASE, record.x, record.y);
  Call(ds, event, DamageService::TITLE_PANEL);
}

uint16_t svc_objdamag(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  auto const arg{ [&](unsigned i) { return Arg(args, count, i); } };
  auto const byte{ [&](unsigned i) { return static_cast<uint8_t>(Arg(args, count, i)); } };
  auto const host{ [] { return static_cast<DialogHost const*>(port_dialogs_host()); } };
  switch (static_cast<DamageService>(id)) {
  case DamageService::LOCATION_BLAST: opeu_location_blast(ds, svc_blast, byte(0), byte(1), byte(2)); return 0;
  case DamageService::FORMAT:         opeu_request_format(ds, { args, count }); return 0;
  case DamageService::SIDE_REPORT:    opeu_objective_side_report(ds, host(), arg(0), arg(1)); return 0;
  case DamageService::ELEMENT_PANEL:  opeu_element_panel(ds, host(), arg(0)); return 0;
  case DamageService::APPEND:         return opeu_string_append(ds, arg(0), arg(1));
  case DamageService::BLAST_ERASE:    opeu_blast_erase(ds, svc_blast, byte(0), byte(1)); return 0;
  case DamageService::TITLE_PANEL:    opeu_title_panel_begin(ds, host()); return 0;
  case DamageService::OBJECTIVE_SIDE: return opeu_objective_side(ds, arg(0));
  case DamageService::OPPOSITE_SIDE:  return opeu_opposite_side(byte(0));
  default:                            port_unhandled("objdamag", id, count);
  }
  return 0;
}
