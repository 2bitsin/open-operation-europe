// The damage report of a bombed objective: the location flashes, each kind of equipment held there loses some of its
// quantity (more on a hit, infantry many times more under heavy bombing) and part of the other losses goes back
// to stock, the three supply stocks lose some, the garrison strength drops, and each loss is reported.
#include "map/objdamag.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/side-color.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "map/blast.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "opeu/contract.hpp"
#include "port.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

using namespace opeu::game;

namespace {
// The report texts the services format: the loss line's, and the damage report's loss and report texts sharing one
// buffer as they shared the routine's frame.
constexpr std::size_t LINE_BYTES{ 0x68 };
constexpr std::size_t REPORT_BYTES{ 0x8e };
constexpr std::size_t LOSS_TEXT_AT{ 0 };
constexpr std::size_t REPORT_AT{ 0x20 };
// The supply stocks an objective keeps (food, fuel, ammunition).
constexpr std::uint8_t SUPPLY_KINDS{ 3 };

} // namespace

// Removes amount..2 * amount - 1 of one of the objective's supply stocks (kind 0..2) and reports it.
void opeu_objective_loss_line(Game& game, uint16_t objective, uint8_t kind, uint16_t amount) {
  auto&                  data{ Data(game) };
  auto&                  record{ Record<Objective>(data, objective) };
  TextBuffer<LINE_BYTES> text;
  opeu::Expects(kind < SUPPLY_KINDS, "a loss line names one of the objective's supply stocks");
  auto const stock{ Near(data, record.resources[kind]) };
  auto const before{ Word(game, stock) };
  uint16_t   value{ before };
  value = SubWord(value, game.State().RandomBounded(amount) + amount);
  SetWord(game, stock, value);
  auto const after{ Word(game, stock) };
  auto const colour{ SideColorCode(static_cast<uint8_t>(opeu_objective_side(game, objective))) };
  opeu_request_format(game, text, std::array<FormatArgument, 7>{ Near(data, data.damage_supply_format), colour, objective, data.supply_names[kind], before - after, data.supply_names[kind], after });
  opeu_objective_side_report(game, objective, text);
}

void opeu_objective_damage(Game& game, uint16_t objective, Bombing const& bombing) {
  auto const [chance, base, spread, heavy, supply, endurance]{ bombing };
  auto&                    data{ Data(game) };
  auto&                    record{ Record<Objective>(data, objective) };
  TextBuffer<REPORT_BYTES> locals;
  auto const               report{ locals.At(REPORT_AT) };
  auto const               loss_text{ locals.At(LOSS_TEXT_AT) };
  auto const               side{ static_cast<uint8_t>(opeu_objective_side(game, objective)) };
  auto const               enemy{ static_cast<uint8_t>(opeu_opposite_side(static_cast<uint8_t>(side))) };
  opeu_location_blast(game, static_cast<uint8_t>(record.x), static_cast<uint8_t>(record.y), static_cast<uint8_t>(enemy));
  if (base) {
    opeu_request_format(game, report, std::array<FormatArgument, 1>{ Near(data, data.damage_title) });
    opeu_request_format(game, report, std::array<FormatArgument, 3>{ Near(data, data.damage_heading), SideColorCode(side), objective });
    opeu_objective_side_report(game, objective, report);
    for (auto& item : record.items) {
      if (item.quantity == 0) continue;
      auto const& type{ data.element_types[item.type] };
      bool const  infantry{ type.category == Bits(Category::INFANTRY) };
      uint16_t    lost{ game.State().RandomBounded(100) >= chance ? game.State().RandomBounded(spread >> 1) : static_cast<uint16_t>(game.State().RandomBounded(spread) + base) };
      if (heavy && infantry) lost = static_cast<uint16_t>(lost * (game.State().RandomBounded(10) + 6));
      lost          = opeu_min16(item.quantity, lost);
      item.quantity = static_cast<uint16_t>(item.quantity - lost);
      opeu_request_format(game, report, std::array<FormatArgument, 4>{ Near(data, data.damage_element_format), SideColorCode(side), data.side_names[side], type.name });
      opeu_element_panel(game, static_cast<std::uint16_t>(Near(data, item)));
      if (lost == 0) {
        opeu_string_append(game, report, Near(data, data.damage_none));
        opeu_objective_side_report(game, objective, report);
        continue;
      }
      opeu_request_format(game, loss_text, std::array<FormatArgument, 2>{ lost == 1 ? Near(data, data.damage_loss_one) : Near(data, data.damage_loss_many), lost });
      opeu_string_append(game, report, loss_text);
      opeu_objective_side_report(game, objective, report);
      if (!infantry) {
        uint16_t stock{ item.extra };
        stock      = AddWord(stock, game.State().RandomBounded(lost / 3), data.request_capacities[type.category]);
        item.extra = stock;
      }
    }
  }
  if (supply) {
    opeu_objective_loss_line(game, objective, 1, supply);
    opeu_objective_loss_line(game, objective, 0, supply);
    opeu_objective_loss_line(game, objective, 2, supply);
  }
  if (endurance) {
    auto const before{ record.strength };
    record.strength = SubByte(record.strength, game.State().RandomBounded(endurance) + endurance);
    opeu_request_format(game, report, std::array<FormatArgument, 5>{ Near(data, data.damage_endurance_format), SideColorCode(side), objective, record.strength, before - record.strength });
    opeu_objective_side_report(game, objective, report);
  }
  opeu_blast_erase(game, static_cast<uint8_t>(record.x), static_cast<uint8_t>(record.y));
  opeu_title_panel_begin(game);
}
