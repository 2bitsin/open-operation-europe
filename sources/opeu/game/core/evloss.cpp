// Hourly unit losses. In scenarios 2-4 the partisans or the resistance strike, one time in a hundred, any ready
// regiment in a category-7 slot or one judged worn out; in scenario 3 in clear or cloudy weather, regiments north of
// row 10 are hit one time in fifty. The side concerned is told through the daily display and the regiment takes
// casualties.
#include "types.hpp"
#include "core/evloss.hpp"
#include "common/events.hpp"

#include "common/data-segment.hpp"
#include "common/scenario.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "glue.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/mapunit.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/casualty.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include "units/value.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the events call through their event.
enum Service : std::uint16_t {
  QUANTITY     = 0, // (unit, category, &count) -> the quantity
  CATEGORY_7   = 1, // (slot) -> nonzero for a category-7 slot
  ELIGIBLE     = 2, // (unit) -> nonzero when the unit can act
  SPECIAL      = 3, // (unit) -> nonzero for a special unit
  UNIT_NAME    = 4, // (unit, buffer) -> the buffer
  LOOKUP       = 5, // (message) -> the text
  FORMAT       = 6, // (destination, format, words...)
  UNIT_MESSAGE = 7, // (unit, text)
  UNIT_REPORT  = 8, // (unit, text) to the other side
  FLASH        = 9, // (x, y, side) the unit's cell on the map
  DAILY_SHOW =
      10, // (id, side, offset, segment) the presentation and its far callback
  MUSIC          = 11, // ()
  CASUALTIES     = 12, // (unit, a, b, c, d, e)
  CUE            = 13, // ()
  PENDING_FINISH = 14, // (value)
};

// The messages of the events.
enum Message : std::uint16_t {
  PARTISANS_STRIKE_ENEMY =
      29, // formatted with the movement and the enemy regiment's name
  PARTISANS_STRIKE_US = 30, // formatted with the movement
  ENEMY_LOSSES_NORTH  = 31, // formatted with the enemy regiment's name
  OUR_LOSSES_NORTH    = 32,
};

constexpr std::uint16_t TEXT{ offsetof(DataSegment, event_buffer) };
constexpr std::uint16_t ALL_CATEGORIES{ 255 };
constexpr std::uint8_t  NORTH_ROWS{ 10 };
// The presentations and the far callbacks (segment and offset of opeu_loss_present_a/b) the daily display runs.
constexpr std::uint16_t PRESENT_SEGMENT{ 0x108c };
constexpr std::uint16_t PRESENT_A_ID{ 0x38 };
constexpr std::uint16_t PRESENT_A{ 0x06ac };
constexpr std::uint16_t PRESENT_B_ID{ 0xb2 };
constexpr std::uint16_t PRESENT_B{ 0x0830 };
// Frame locals whose address the services receive, below the event routine's frame pointer.
constexpr std::uint16_t WEAK_FRAME{
  0x1a
}; // opeu_loss_unit_weak's frame below event a's
constexpr std::uint16_t WEAK_COUNT{ 0xc };
constexpr std::uint16_t NAME_A{ 0x14 };
constexpr std::uint16_t NAME_B{ 0x10 };

// The format with one word, the word computed before the format is looked up.
auto Format(std::uint8_t* ds, HostEvent event, std::uint16_t message,
            std::uint16_t word) -> void {
  auto const format{ Call(ds, event, LOOKUP, message) };
  Call(ds, event, FORMAT, TEXT, format, word);
}
} // namespace

// Whether the unit is worn out: by random thresholds on its elements' average morale and experience, or on its
// strength (infantry counting a tenth).
uint16_t opeu_loss_unit_weak(uint8_t ds[65536], HostEvent event, uint16_t bp,
                             uint16_t unit) {
  auto&         data{ Data(ds) };
  std::uint16_t morale{ 0 };
  std::uint16_t experience{ 0 };
  std::uint16_t strength{ 0 };
  for (auto const& element : Elements(data, Record<Unit>(data, unit).first)) {
    morale += element.morale;
    experience += element.experience;
    strength += data.element_types[element.type].category == 0
                    ? element.quantity / 10
                    : element.quantity;
  }
  auto const at_count{ Local(bp, WEAK_COUNT) };
  Call(ds, event, QUANTITY, unit, ALL_CATEGORIES, at_count);
  auto const count{ static_cast<std::uint8_t>(Word(ds, at_count)) };
  if (!count)
    return 0;
  if ((RandomBelow(ds, 20) + 20 > morale / count &&
       experience / count > 50 - RandomBelow(ds, 20)) ||
      RandomBelow(ds, 20) + 20 > strength)
    return 1;
  return 0;
}

void opeu_loss_present_a(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, CUE);
  Call(ds, event, PENDING_FINISH, 3);
}

void opeu_loss_event_a(uint8_t ds[65536], HostEvent event, uint16_t bp) {
  auto&         data{ Data(ds) };
  auto const    name{ Local(bp, NAME_A) };
  auto const    scenario{ CurrentScenario(data) };
  std::uint16_t movement{};
  if (scenario == Scenario::KURSK)
    movement = data.resistance_names[0];
  else if (scenario == Scenario::D_DAY || scenario == Scenario::BULGE)
    movement = data.resistance_names[1];
  else
    return;
  for (unsigned group{ 0 }; group < GROUPS_PER_SIDE; ++group)
    for (unsigned slot{ 0 }; slot < UNITS_PER_GROUP; ++slot) {
      auto const near{ Near(data, data.units[group * UNITS_PER_GROUP + slot]) };
      if (!(Record<Unit>(data, near).flags & Bits(UnitFlag::READY)))
        continue;
      if (!Call(ds, event, CATEGORY_7, slot) &&
          !opeu_loss_unit_weak(
              ds, event, static_cast<std::uint16_t>(bp - WEAK_FRAME), near))
        continue;
      if (RandomBelow(ds, 100))
        continue;
      if (data.side_status[1] == SIDE_PLAYED &&
          Call(ds, event, SPECIAL, near)) {
        auto const unit_name{ Call(ds, event, UNIT_NAME, near, name) };
        auto const format{ Call(ds, event, LOOKUP, PARTISANS_STRIKE_ENEMY) };
        Call(ds, event, FORMAT, TEXT, format, movement, unit_name);
        Call(ds, event, UNIT_REPORT, near, TEXT);
        auto const& unit{ Record<Unit>(data, near) };
        Call(ds, event, FLASH, unit.x, unit.y, 1);
        Call(ds, event, DAILY_SHOW, PRESENT_A_ID, 1, PRESENT_A,
             PRESENT_SEGMENT);
      } else if (data.side_status[0] == SIDE_PLAYED &&
                 Call(ds, event, ELIGIBLE, near)) {
        Format(ds, event, PARTISANS_STRIKE_US, movement);
        Call(ds, event, UNIT_MESSAGE, near, TEXT);
        Call(ds, event, DAILY_SHOW, PRESENT_A_ID, 0, PRESENT_A,
             PRESENT_SEGMENT);
      }
      Call(ds, event, MUSIC);
      Call(ds, event, CASUALTIES, near, 10, 10, 5, 1, 1);
    }
}

// Whether the unit stands north of row 10.
uint16_t opeu_loss_unit_low(uint8_t ds[65536], uint16_t unit) {
  return Record<Unit>(Data(ds), unit).y < NORTH_ROWS ? 1 : 0;
}

void opeu_loss_present_b(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, PENDING_FINISH, 3);
}

void opeu_loss_event_b(uint8_t ds[65536], HostEvent event, uint16_t bp) {
  auto&      data{ Data(ds) };
  auto const name{ Local(bp, NAME_B) };
  if (CurrentScenario(data) != Scenario::D_DAY ||
      (data.weather != 0 && data.weather != 1))
    return;
  for (unsigned i{ 0 }; i < UNITS_PER_SIDE; ++i) {
    auto const near{ Near(data, data.units[i]) };
    if (!(Record<Unit>(data, near).flags & Bits(UnitFlag::READY)) ||
        !opeu_loss_unit_low(ds, near) || RandomBelow(ds, 50))
      continue;
    if (data.side_status[1] == SIDE_PLAYED && Call(ds, event, SPECIAL, near)) {
      Format(ds, event, ENEMY_LOSSES_NORTH,
             Call(ds, event, UNIT_NAME, near, name));
      Call(ds, event, UNIT_REPORT, near, TEXT);
      auto const& unit{ Record<Unit>(data, near) };
      Call(ds, event, FLASH, unit.x, unit.y, 1);
      Call(ds, event, DAILY_SHOW, PRESENT_B_ID, 1, PRESENT_B, PRESENT_SEGMENT);
    } else if (data.side_status[0] == SIDE_PLAYED &&
               Call(ds, event, ELIGIBLE, near)) {
      Call(ds, event, UNIT_MESSAGE, near,
           Call(ds, event, LOOKUP, OUR_LOSSES_NORTH));
      Call(ds, event, DAILY_SHOW, PRESENT_B_ID, 0, PRESENT_B, PRESENT_SEGMENT);
    }
    Call(ds, event, CASUALTIES, near, 20, 20, 5, 0, 1);
  }
}

uint16_t svc_evloss(uint8_t* ds, uint16_t id, const uint16_t* args,
                    uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
    case QUANTITY:
      result = opeu_unit_quantity(ds, Arg(args, count, 0),
                                  static_cast<uint8_t>(Arg(args, count, 1)),
                                  PORT_DS(Arg(args, count, 2)));
      break;
    case CATEGORY_7:
      result = opeu_category_7(static_cast<uint8_t>(Arg(args, count, 0)));
      break;
    case ELIGIBLE:
      result = opeu_unit_selection_eligible(ds, Arg(args, count, 0));
      break;
    case SPECIAL: result = opeu_unit_special(ds, Arg(args, count, 0)); break;
    case UNIT_NAME:
      result = opeu_unit_name(ds, svc_unitname, Arg(args, count, 0),
                              Arg(args, count, 1));
      break;
    case LOOKUP:
      result =
          opeu_message_lookup(ds, svc_msglook, (int16_t)Arg(args, count, 0));
      break;
    case FORMAT: c1_request_format(ds, args, count); break;
    case UNIT_MESSAGE:
      opeu_unit_report(ds, (const DialogHost*)port_dialogs_host(),
                       Arg(args, count, 0), Arg(args, count, 1));
      break;
    case UNIT_REPORT:
      opeu_unit_side_report(ds, (const DialogHost*)port_dialogs_host(),
                            Arg(args, count, 0), Arg(args, count, 1));
      break;
    case FLASH: {
      auto const frame{ port_frame_enter() };
      opeu_location_flash(ds, svc_mapunit, frame,
                          static_cast<uint8_t>(Arg(args, count, 0)),
                          static_cast<uint8_t>(Arg(args, count, 1)),
                          static_cast<uint8_t>(Arg(args, count, 2)));
      port_frame_leave();
      break;
    }
    case DAILY_SHOW:
      port_daily_show(ds, Arg(args, count, 0), Arg(args, count, 1),
                      Arg(args, count, 2), Arg(args, count, 3));
      break;
    case MUSIC:      opeu_music_command7(ds, svc_sound); break;
    case CASUALTIES: {
      auto const frame{ port_frame_enter() };
      opeu_unit_casualties(ds, svc_casualty, frame, Arg(args, count, 0),
                           Arg(args, count, 1), Arg(args, count, 2),
                           Arg(args, count, 3), Arg(args, count, 4),
                           Arg(args, count, 5));
      port_frame_leave();
      break;
    }
    case CUE: opeu_sound_cue12(ds, svc_soundcue); break;
    case PENDING_FINISH:
      opeu_pending_finish(ds, svc_waitkey,
                          static_cast<uint8_t>(Arg(args, count, 0)));
      break;
    default: port_unhandled("evloss", id, count);
  }
  return result;
}

auto port_routine_opeu_loss_event_a(std::uint8_t* ds, std::uint16_t const*,
                                    unsigned) -> std::uint32_t {
  WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    opeu_loss_event_a(ds, svc_evloss, static_cast<std::uint16_t>(sp - 2));
  });
  return 0;
}

auto port_routine_opeu_loss_event_b(std::uint8_t* ds, std::uint16_t const*,
                                    unsigned) -> std::uint32_t {
  WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    opeu_loss_event_b(ds, svc_evloss, static_cast<std::uint16_t>(sp - 2));
  });
  return 0;
}

auto port_routine_opeu_loss_present_a(std::uint8_t* ds, std::uint16_t const*,
                                      unsigned) -> std::uint32_t {
  opeu_loss_present_a(ds, svc_evloss);
  return 0;
}

auto port_routine_opeu_loss_present_b(std::uint8_t* ds, std::uint16_t const*,
                                      unsigned) -> std::uint32_t {
  opeu_loss_present_b(ds, svc_evloss);
  return 0;
}
