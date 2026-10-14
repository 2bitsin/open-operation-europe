// Casualties for every element of a regiment: each loses men at random, survivors gain experience and lose morale,
// a wiped-out element leaves the regiment and the selection is repaired. A side a player commands sees every loss
// reported; the regiment's resource percentages are then recomputed.
#include "units/casualty.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
#include "common/side-color.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/counter.hpp"
#include "core/dialogs.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "map/blast.hpp"
#include "map/mappos.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/capacity.hpp"
#include "units/elemlink.hpp"
#include "units/genwound.hpp"
#include "units/labels.hpp"
#include "units/percent.hpp"
#include "units/select.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include <algorithm>
#include <array>

using namespace opeu::game;

namespace {

// The kinds of person SIDE_RECORD finds: the side's commander, its chief of staff and its general.
constexpr std::uint8_t SIDE_PEOPLE{ 3 };
// REMOVE_ELEMENT's results.
constexpr std::uint8_t UNIT_EMPTIED{ 1 };
constexpr std::uint8_t GROUP_EMPTIED{ 3 };
// No side shown in the overview or tactical view: the next view redraws.
constexpr std::uint8_t NO_VIEW{ 0xff };

// The routine's texts, sharing one buffer at their places below its top as they shared the routine's frame.
constexpr std::uint16_t LOCALS_BYTES{ 0xee };
enum Locals : std::uint16_t {
  UNIT_NAME_TEXT    = 0x2a, // 32 bytes
  ELEMENT_NAME_TEXT = 0x52, // 32 bytes
  LOSS_TEXT         = 0xee, // 30 bytes
  REPORT_TEXT       = 0xce, // 100 bytes
};

// The morale an element loses: the square root of its losses, times one more than a twentieth of its experience, plus
// up to four. The random draw comes before the root, as in the game.
auto MoraleLoss(Game& game, std::uint16_t losses, Element const& element) -> std::uint16_t {
  auto const extra{ game.State().RandomBounded(5) };
  auto const root{ opeu_isqrt(losses) };
  return static_cast<std::uint16_t>(root * (element.experience / 20 + 1) + extra);
}
} // namespace

void opeu_unit_casualties(Game& game, uint16_t unit, CasualtyRoll const& roll) {
  auto const [percent, kind, c, d, e]{ roll };
  auto&                    data{ Data(game) };
  auto&                    regiment{ Record<Unit>(data, unit) };
  TextBuffer<LOCALS_BYTES> locals;
  auto const               local{ [&](Locals offset) { return locals.At(LOCALS_BYTES - offset); } };
  auto const               name{ local(UNIT_NAME_TEXT) };
  auto const               element_name{ local(ELEMENT_NAME_TEXT) };
  auto const               text{ local(REPORT_TEXT) };
  auto const               loss_text{ local(LOSS_TEXT) };
  std::uint8_t const       x{ regiment.x };
  std::uint8_t const       y{ regiment.y };
  auto const               side{ static_cast<std::uint8_t>(opeu_unit_side(unit)) };
  auto const               group{ static_cast<std::uint8_t>(opeu_unit_group_index(unit)) };
  auto const               division{ Near(data, GroupOf(data, side, group)) };
  bool const               side_selected{ opeu_find_side_selected_unit(game, static_cast<uint8_t>(side)) == unit };
  bool const               group_selected{ opeu_find_group_selected_unit(game, division) == unit };
  opeu_unit_name(game, unit, name);
  // The side that watches: the enemy when a player commands it and may see the regiment, else its own side when a
  // player commands it and the regiment may be selected.
  std::uint8_t viewer{ opeu_opposite_side(side) };
  bool         visible{ data.side_status[viewer] == SIDE_PLAYED && opeu_unit_special(game, unit) };
  if (!visible) {
    viewer  = side;
    visible = data.side_status[viewer] == SIDE_PLAYED && opeu_unit_selection_eligible(game, unit);
  }
  if (visible && e) opeu_location_blast(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(viewer));
  auto const percents{ Figures(UnitResourcePercentages(data, unit)) };
  bool       report{ true };
  for (auto near{ regiment.first }; near; near = Record<Element>(data, near).next) {
    auto& element{ Record<Element>(data, near) };
    opeu_element_name(game, near, element_name);
    auto const    category{ data.element_types[element.type].category };
    bool const    infantry{ category == 0 };
    std::uint16_t losses{ game.State().RandomBounded(100) >= percent ? game.State().RandomBounded(c >> 1) : static_cast<std::uint16_t>(game.State().RandomBounded(c) + kind) };
    if (d && infantry) losses = static_cast<std::uint16_t>(static_cast<std::uint32_t>(losses) * static_cast<std::uint32_t>(game.State().RandomBounded(10) + 6));
    losses = opeu_min16(element.quantity, losses);
    opeu_request_format(game, text, std::array<FormatArgument, 5>{ Near(data, data.casualty_heading), SideColorCode(side), data.side_names[side], name, element_name });
    bool wiped{ false };
    if (losses) {
      element.quantity -= losses;
      if (element.flags & Bits(ElementFlag::SELECTED)) {
        if (side_selected)
          for (std::uint8_t person{ 1 }; person <= SIDE_PEOPLE; ++person) opeu_general_casualty(game, opeu_find_side_record(game, static_cast<uint8_t>(side), static_cast<uint8_t>(person)));
        if (group_selected) opeu_general_casualty(game, opeu_find_group_record(game, static_cast<uint8_t>(side), static_cast<uint8_t>(group)));
        // One time in a hundred the regiment's communications are cut.
        if (game.State().RandomBounded(100) == 0 && opeu_unit_selection_eligible(game, unit)) {
          if (data.side_status[side] == SIDE_PLAYED) {
            if (group_selected)
              opeu_string_copy(game, text, Near(data, data.casualty_cut_selected));
            else if (!side_selected)
              opeu_request_format(game, text, std::array<FormatArgument, 2>{ Near(data, data.casualty_cut), name });
            opeu_unit_side_report(game, unit, text);
            report = false;
          }
          regiment.flags &= static_cast<std::uint8_t>(~Bits(UnitFlag::COMMUNICATIONS));
          data.view_side = data.overview_side = NO_VIEW;
          opeu_arrival_location(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(side));
        }
      }
      if (element.quantity) {
        element.experience = AddByte(element.experience, static_cast<std::uint16_t>(opeu_isqrt(kind) * 2), RATING_LIMIT);
        element.morale     = SubByte(element.morale, MoraleLoss(game, infantry ? losses / 10 : losses, element));
        if (!infantry) {
          std::uint16_t damage{ element.damage };
          damage         = AddWord(damage, game.State().RandomBounded(losses / 3), data.request_capacities[category]);
          element.damage = damage;
        }
        opeu_request_format(game, loss_text, std::array<FormatArgument, 2>{ losses == 1 ? Near(data, data.casualty_loss_format) : Near(data, data.casualty_losses_format), losses });
        opeu_string_append(game, text, loss_text);
      } else {
        switch (static_cast<std::uint8_t>(opeu_remove_element(game, unit, near))) {
        case UNIT_EMPTIED:
          if (group_selected) opeu_select_first_unit(game, division);
          wiped = true;
          break;
        case GROUP_EMPTIED:
          opeu_set_record_counter(Record<Person>(Data(game), static_cast<std::uint16_t>(opeu_find_group_record(game, static_cast<uint8_t>(side), static_cast<uint8_t>(group)))), 0);
          if (side_selected) opeu_select_first_group(game, static_cast<uint8_t>(side));
          wiped = true;
          break;
        default:
          if (element.flags & Bits(ElementFlag::SELECTED)) opeu_select_preferred_element(game, unit);
          break;
        }
        opeu_string_append(game, text, Near(data, data.casualty_wiped));
      }
    } else
      opeu_string_append(game, text, Near(data, data.casualty_no_damage));
    if (report) {
      if (visible) opeu_group_element_panel(game, near);
      if (wiped) {
        if (visible) {
          auto const person{ side == viewer ? opeu_find_group_record(game, static_cast<uint8_t>(viewer), static_cast<uint8_t>(group))
                                            : opeu_find_side_record(game, static_cast<uint8_t>(viewer), static_cast<uint8_t>(SIDE_PEOPLE)) };
          opeu_arrival_location(game, static_cast<uint8_t>(regiment.x), static_cast<uint8_t>(regiment.y), static_cast<uint8_t>(viewer));
          opeu_control_message(game, text, person);
        }
      } else
        opeu_unit_side_report(game, unit, text);
    }
  }
  regiment.moved = 0;
  std::ranges::transform(Figures(UnitCapacities(data, unit)), percents, regiment.resources, opeu_percent);
  if (visible && e) opeu_blast_erase(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
}
