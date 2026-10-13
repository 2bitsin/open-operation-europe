// Casualties for every element of a regiment: each loses men at random, survivors gain experience and lose morale,
// a wiped-out element leaves the regiment and the selection is repaired. A side a player commands sees every loss
// reported; the regiment's resource percentages are then recomputed.
#include "units/casualty.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "core/counter.hpp"
#include "core/dialogs.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "glue.hpp"
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

using namespace opeu::game;

namespace {
// The services the casualty routine calls through its event (svc_casualty).
enum class Service : std::uint16_t {
  UNIT_SIDE,            // (unit) -> side
  UNIT_GROUP_INDEX,     // (unit) -> its division's index on its side
  SIDE_SELECTED_UNIT,   // (side) -> the regiment selected on the side
  GROUP_SELECTED_UNIT,  // (division) -> the regiment selected in the division
  UNIT_NAME,            // (unit, buffer)
  UNIT_SPECIAL,         // (unit) -> nonzero when the enemy may see it
  UNIT_ELIGIBLE,        // (unit) -> nonzero when it may be selected
  LOCATION_BLAST,       // (x, y, side): the blast animation at a cell
  RESOURCE_PERCENTAGES, // (unit, ammunition, fuel, food): words receiving the percentages
  ELEMENT_NAME,         // (element, buffer)
  RANDOM,               // (bound) -> 0..bound-1
  FORMAT,               // (destination, format, ...)
  SIDE_RECORD,          // (side, kind) -> the side's commander, chief of staff or general
  GENERAL_CASUALTY,     // (person): a general may be wounded
  GROUP_RECORD,         // (side, division) -> the division's general
  SIDE_REPORT,          // (unit, text)
  ARRIVAL_LOCATION,     // (x, y, side): move the view to a cell
  SQUARE_ROOT,          // (value)
  REMOVE_ELEMENT,       // (unit, element) -> 1 the unit emptied, 3 the division emptied
  SELECT_ELEMENT,       // (unit): select its preferred element
  SELECT_UNIT,          // (division): select its first regiment
  SET_COUNTER,          // (person, count)
  SELECT_GROUP,         // (side): select its first division
  REPORT_ELEMENT,       // (element): the element panel
  CONTROL_MESSAGE,      // (text, person)
  CAPACITIES,           // (unit, ammunition, fuel, food): words receiving the capacities
  BLAST_ERASE,          // (x, y)
};

// The kinds of person SIDE_RECORD finds: the side's commander, its chief of staff and its general.
constexpr std::uint8_t SIDE_PEOPLE{ 3 };
// REMOVE_ELEMENT's results.
constexpr std::uint8_t UNIT_EMPTIED{ 1 };
constexpr std::uint8_t GROUP_EMPTIED{ 3 };
// No side shown in the overview or tactical view: the next view redraws.
constexpr std::uint8_t NO_VIEW{ 0xff };

// The routine's locals whose near addresses reach services, at their places below the top of its reserve.
constexpr std::uint16_t LOCALS_BYTES{ 0xee };
enum Locals : std::uint16_t {
  PERCENT_AMMUNITION  = 0x2c,
  UNIT_NAME_TEXT      = 0x2a, // 32 bytes
  CAPACITY_AMMUNITION = 0x2e,
  ELEMENT_NAME_TEXT   = 0x52, // 32 bytes
  CAPACITY_FOOD       = 0x5a,
  PERCENT_FOOD        = 0x58,
  LOSS_TEXT           = 0xee, // 30 bytes
  REPORT_TEXT         = 0xce, // 100 bytes
  CAPACITY_FUEL       = 0x68,
  PERCENT_FUEL        = 0x66,
};

// The morale an element loses: the square root of its losses, times one more than a twentieth of its experience, plus
// up to four. The random draw comes before the root, as in the game.
auto MoraleLoss(std::uint8_t* ds, HostEvent event, std::uint16_t losses, Element const& element) -> std::uint16_t {
  auto const extra{ Call(ds, event, Service::RANDOM, 5) };
  auto const root{ Call(ds, event, Service::SQUARE_ROOT, losses) };
  return static_cast<std::uint16_t>(root * (element.experience / 20 + 1) + extra);
}
} // namespace

// unit suffers casualties: each element loses up to c men plus kind (percent% of the time, otherwise up to c/2);
// infantry (category 0) loses six to fifteen times that when d is set. e shows the blast animation.
void opeu_unit_casualties(uint8_t ds[65536], HostEvent event, uint16_t unit, uint16_t percent, uint16_t kind, uint16_t c, uint16_t d, uint16_t e) {
  auto&               data{ Data(ds) };
  auto&               regiment{ Record<Unit>(data, unit) };
  ServiceLocals const locals{ LOCALS_BYTES };
  auto const          local{ [&](Locals offset) { return locals.At(static_cast<std::uint16_t>(LOCALS_BYTES - offset)); } };
  auto const          name{ local(UNIT_NAME_TEXT) };
  auto const          element_name{ local(ELEMENT_NAME_TEXT) };
  auto const          text{ local(REPORT_TEXT) };
  auto const          loss_text{ local(LOSS_TEXT) };
  auto const          percents{
    std::array{ local(PERCENT_AMMUNITION), local(PERCENT_FUEL), local(PERCENT_FOOD) }
  };
  auto const capacities{
    std::array{ local(CAPACITY_AMMUNITION), local(CAPACITY_FUEL), local(CAPACITY_FOOD) }
  };
  std::uint8_t const x{ regiment.x };
  std::uint8_t const y{ regiment.y };
  auto const         side{ static_cast<std::uint8_t>(Call(ds, event, Service::UNIT_SIDE, unit)) };
  auto const         group{ static_cast<std::uint8_t>(Call(ds, event, Service::UNIT_GROUP_INDEX, unit)) };
  auto const         division{ Near(data, GroupOf(data, side, group)) };
  bool const         side_selected{ Call(ds, event, Service::SIDE_SELECTED_UNIT, side) == unit };
  bool const         group_selected{ Call(ds, event, Service::GROUP_SELECTED_UNIT, division) == unit };
  Call(ds, event, Service::UNIT_NAME, unit, name);
  // The side that watches: the enemy when a player commands it and may see the regiment, else its own side when a
  // player commands it and the regiment may be selected.
  std::uint8_t viewer{ opeu_opposite_side(side) };
  bool         visible{ data.side_status[viewer] == SIDE_PLAYED && Call(ds, event, Service::UNIT_SPECIAL, unit) };
  if (!visible) {
    viewer  = side;
    visible = data.side_status[viewer] == SIDE_PLAYED && Call(ds, event, Service::UNIT_ELIGIBLE, unit);
  }
  if (visible && e) Call(ds, event, Service::LOCATION_BLAST, x, y, viewer);
  Call(ds, event, Service::RESOURCE_PERCENTAGES, unit, percents[0], percents[1], percents[2]);
  bool report{ true };
  for (auto near{ regiment.first }; near; near = Record<Element>(data, near).next) {
    auto& element{ Record<Element>(data, near) };
    Call(ds, event, Service::ELEMENT_NAME, near, element_name);
    auto const    category{ data.element_types[element.type].category };
    bool const    infantry{ category == 0 };
    std::uint16_t losses{ Call(ds, event, Service::RANDOM, 100) >= percent ? Call(ds, event, Service::RANDOM, c >> 1) : static_cast<std::uint16_t>(Call(ds, event, Service::RANDOM, c) + kind) };
    if (d && infantry) losses = static_cast<std::uint16_t>(static_cast<std::uint32_t>(losses) * static_cast<std::uint32_t>(Call(ds, event, Service::RANDOM, 10) + 6));
    losses = opeu_min16(element.quantity, losses);
    Call(ds, event, Service::FORMAT, text, Near(data, data.casualty_heading), side == 0 ? '6' : '5', data.side_names[side], name, element_name);
    bool wiped{ false };
    if (losses) {
      element.quantity -= losses;
      if (element.flags & Bits(ElementFlag::SELECTED)) {
        if (side_selected)
          for (std::uint8_t person{ 1 }; person <= SIDE_PEOPLE; ++person) Call(ds, event, Service::GENERAL_CASUALTY, Call(ds, event, Service::SIDE_RECORD, side, person));
        if (group_selected) Call(ds, event, Service::GENERAL_CASUALTY, Call(ds, event, Service::GROUP_RECORD, side, group));
        // One time in a hundred the regiment's communications are cut.
        if (Call(ds, event, Service::RANDOM, 100) == 0 && Call(ds, event, Service::UNIT_ELIGIBLE, unit)) {
          if (data.side_status[side] == SIDE_PLAYED) {
            if (group_selected)
              opeu_string_copy(ds, text, Near(data, data.casualty_cut_selected));
            else if (!side_selected)
              Call(ds, event, Service::FORMAT, text, Near(data, data.casualty_cut), name);
            Call(ds, event, Service::SIDE_REPORT, unit, text);
            report = false;
          }
          regiment.flags &= static_cast<std::uint8_t>(~Bits(UnitFlag::COMMUNICATIONS));
          data.view_side = data.overview_side = NO_VIEW;
          Call(ds, event, Service::ARRIVAL_LOCATION, x, y, side);
        }
      }
      if (element.quantity) {
        opeu_add_byte(static_cast<std::uint16_t>(Call(ds, event, Service::SQUARE_ROOT, kind) * 2), RATING_LIMIT, &element.experience);
        opeu_sub_byte(MoraleLoss(ds, event, infantry ? losses / 10 : losses, element), &element.morale);
        if (!infantry) {
          std::uint16_t damage{ element.damage };
          opeu_add_word(Call(ds, event, Service::RANDOM, losses / 3), data.request_capacities[category], &damage);
          element.damage = damage;
        }
        Call(ds, event, Service::FORMAT, loss_text, losses == 1 ? Near(data, data.casualty_loss_format) : Near(data, data.casualty_losses_format), losses);
        opeu_string_append(ds, text, loss_text);
      } else {
        switch (static_cast<std::uint8_t>(Call(ds, event, Service::REMOVE_ELEMENT, unit, near))) {
        case UNIT_EMPTIED:
          if (group_selected) Call(ds, event, Service::SELECT_UNIT, division);
          wiped = true;
          break;
        case GROUP_EMPTIED:
          Call(ds, event, Service::SET_COUNTER, Call(ds, event, Service::GROUP_RECORD, side, group), 0);
          if (side_selected) Call(ds, event, Service::SELECT_GROUP, side);
          wiped = true;
          break;
        default:
          if (element.flags & Bits(ElementFlag::SELECTED)) Call(ds, event, Service::SELECT_ELEMENT, unit);
          break;
        }
        opeu_string_append(ds, text, Near(data, data.casualty_wiped));
      }
    } else
      opeu_string_append(ds, text, Near(data, data.casualty_no_damage));
    if (report) {
      if (visible) Call(ds, event, Service::REPORT_ELEMENT, near);
      if (wiped) {
        if (visible) {
          auto const person{ side == viewer ? Call(ds, event, Service::GROUP_RECORD, viewer, group) : Call(ds, event, Service::SIDE_RECORD, viewer, SIDE_PEOPLE) };
          Call(ds, event, Service::ARRIVAL_LOCATION, regiment.x, regiment.y, viewer);
          Call(ds, event, Service::CONTROL_MESSAGE, text, person);
        }
      } else
        Call(ds, event, Service::SIDE_REPORT, unit, text);
    }
  }
  regiment.moved = 0;
  Call(ds, event, Service::CAPACITIES, unit, capacities[0], capacities[1], capacities[2]);
  for (std::size_t resource{ 0 }; resource < 3; ++resource) regiment.resources[resource] = opeu_percent(Word(ds, capacities[resource]), Word(ds, percents[resource]));
  if (visible && e) Call(ds, event, Service::BLAST_ERASE, x, y);
}

// The casualty routine's services.
uint16_t svc_casualty(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0, a, b, c;
  switch (static_cast<Service>(id)) {
  case Service::UNIT_SIDE:           r = opeu_unit_side(Arg(args, count, 0)); break;
  case Service::UNIT_GROUP_INDEX:    r = opeu_unit_group_index(Arg(args, count, 0)); break;
  case Service::SIDE_SELECTED_UNIT:  r = opeu_find_side_selected_unit(ds, (uint8_t)Arg(args, count, 0)); break;
  case Service::GROUP_SELECTED_UNIT: r = opeu_find_group_selected_unit(ds, Arg(args, count, 0)); break;
  case Service::UNIT_NAME:           r = opeu_unit_name(ds, svc_unitname, Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::UNIT_SPECIAL:        r = opeu_unit_special(ds, Arg(args, count, 0)); break;
  case Service::UNIT_ELIGIBLE:       r = opeu_unit_selection_eligible(ds, Arg(args, count, 0)); break;
  case Service::LOCATION_BLAST:      opeu_location_blast(ds, svc_blast, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case Service::RESOURCE_PERCENTAGES:
    a = port_ds_word(Arg(args, count, 1));
    b = port_ds_word(Arg(args, count, 2));
    c = port_ds_word(Arg(args, count, 3));
    opeu_unit_resource_percentages(ds, Arg(args, count, 0), &a, &b, &c);
    port_ds_set_word(Arg(args, count, 1), a);
    port_ds_set_word(Arg(args, count, 2), b);
    port_ds_set_word(Arg(args, count, 3), c);
    break;
  case Service::ELEMENT_NAME:     r = opeu_element_name(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::RANDOM:           r = port_rand_bounded(ds, Arg(args, count, 0)); break;
  case Service::FORMAT:           opeu_request_format(ds, { args, count }); break;
  case Service::SIDE_RECORD:      r = opeu_find_side_record(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case Service::GENERAL_CASUALTY: opeu_general_casualty(ds, svc_genwound, Arg(args, count, 0)); break;
  case Service::GROUP_RECORD:     r = opeu_find_group_record(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case Service::SIDE_REPORT:      opeu_unit_side_report(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::ARRIVAL_LOCATION: opeu_arrival_location(ds, svc_mappos, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case Service::SQUARE_ROOT:      r = opeu_isqrt(Arg(args, count, 0)); break;
  case Service::REMOVE_ELEMENT:   r = opeu_remove_element(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::SELECT_ELEMENT:   opeu_select_preferred_element(ds, Arg(args, count, 0)); break;
  case Service::SELECT_UNIT:      opeu_select_first_unit(ds, Arg(args, count, 0)); break;
  case Service::SET_COUNTER:      opeu_set_record_counter(PORT_DS(Arg(args, count, 0)), (uint8_t)Arg(args, count, 1)); break;
  case Service::SELECT_GROUP:     opeu_select_first_group(ds, (uint8_t)Arg(args, count, 0)); break;
  case Service::REPORT_ELEMENT:   opeu_group_element_panel(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case Service::CONTROL_MESSAGE:  opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case Service::CAPACITIES:
    a = port_ds_word(Arg(args, count, 1));
    b = port_ds_word(Arg(args, count, 2));
    c = port_ds_word(Arg(args, count, 3));
    opeu_unit_capacities(ds, Arg(args, count, 0), &a, &b, &c);
    port_ds_set_word(Arg(args, count, 1), a);
    port_ds_set_word(Arg(args, count, 2), b);
    port_ds_set_word(Arg(args, count, 3), c);
    break;
  case Service::BLAST_ERASE: opeu_blast_erase(ds, svc_blast, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  default:                   port_unhandled("casualty", id, count);
  }
  return r;
}
