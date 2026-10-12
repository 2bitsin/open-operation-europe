// Hourly wound recovery of both sides' generals: a wounded general recovers or loses health, and at no health a
// deputy holds on, a role-1 general becomes a deputy and anyone else dies and is replaced by a new deputy. Every
// general then heals a little, up to his health limit.
#include "types.hpp"
#include "core/evwound.hpp"
#include "core/counter.hpp"
#include "core/deputy.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/personfl.hpp"
#include "core/strings.hpp"
#include "types/events.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/dialogs.hpp"
#include "glue.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

// The services the event calls through its event.
enum Service : std::uint16_t {
  DELAY   = 0, // (units) the pause of the death presentation
  CONTEXT = 1, // (value) the turn context
  FINISH  = 2, // (value) the side's initialization
  SHOW    = 3, // (id, side, presenter offset, presenter segment)
  MESSAGE = 4, // (text, person)
};

constexpr std::uint16_t DEATH_PREFIX{ offsetof(DataSegment, death_prefix) };
constexpr std::uint16_t DEATH_SUFFIX{ offsetof(DataSegment, death_suffix) };
constexpr std::uint16_t DEATH_SHOW{ 300 };
constexpr std::uint16_t DEATH_CONTEXT{ 0x10 };
constexpr std::uint16_t PRESENT_OFFSET{
  0x0004
}; // opeu_death_present as the presentation's far routine
constexpr std::uint16_t PRESENT_SEGMENT{ 0x11e5 };
constexpr std::uint16_t SHOWN{
  1
}; // DataSegment::side_status of a side the player watches
constexpr std::uint16_t PLAYER{
  1
}; // DataSegment::side_control of a side the player commands
constexpr std::uint16_t COMPUTER_LEVEL{ 2 }; // the level of a computer side
constexpr std::uint8_t  ADVISER{ 3 }; // the side record kind of the adviser
constexpr std::uint8_t  GENERAL_ROLE{
  1
}; // the role that becomes a deputy instead of dying

// The death notice in the dying general's routine frame: "Commander " + name + suffix, and the side right after it.
// A name of seven or more characters runs the text into the side byte, and the second status test then reads the
// overwritten side.
struct DeathNotice {
  std::array<char, 50> text;
  std::uint8_t         side;
};
constexpr std::uint16_t NOTICE_BELOW_BP{ 0x34 };
constexpr std::uint16_t DEATH_BP_BELOW_SP{
  0x12
}; // the frame pointer opeu_commander_death gets from the event
} // namespace

// The presentation's far routine: a pause.
void opeu_death_present(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, DELAY, 30);
}

// Replaces the dead general with a new deputy; a side the player watches is told by its adviser.
void opeu_commander_death(uint8_t ds[65536], HostEvent event, uint16_t bp,
                          uint16_t person) {
  auto&      data{ Data(ds) };
  auto const at{ Local(bp, NOTICE_BELOW_BP) };
  auto&      notice{ Record<DeathNotice>(data, at) };
  notice.side = opeu_person_lacks_side0_flag(ds, person);
  if (data.side_status[notice.side] == SHOWN) {
    opeu_string_copy(ds, at, DEATH_PREFIX);
    opeu_string_append(ds, at, person);
    opeu_string_append(ds, at, DEATH_SUFFIX);
  }
  opeu_initialize_deputy(ds, person);
  if (data.side_status[notice.side] == SHOWN) {
    Call(ds, event, CONTEXT, DEATH_CONTEXT);
    Call(ds, event, FINISH, 0);
    Call(ds, event, SHOW, DEATH_SHOW, notice.side, PRESENT_OFFSET,
         PRESENT_SEGMENT);
    Call(ds, event, MESSAGE, at,
         opeu_find_side_record(ds, notice.side, ADVISER));
  }
}

// A wounded general recovers with chance 1 in (level * 10 + 24), the level being the delivery level for a side the
// player commands and 2 otherwise; else he loses 10 - limit / 10 health. Healing: 1 for a general assigned to a
// division, 3 while his record counter runs, else 2.
void opeu_wound_event(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto& data{ Data(ds) };
  for (std::uint8_t side{ 0 }; side < SIDE_COUNT; ++side) {
    std::uint16_t const level{ data.side_control[side] == PLAYER
                                   ? data.delivery_level
                                   : COMPUTER_LEVEL };
    for (std::uint8_t i{ 0 }; i < PEOPLE_PER_SIDE; ++i) {
      auto& person{ data.people[side * PEOPLE_PER_SIDE + i] };
      if (person.status == PERSON_VACANT)
        continue;
      if (person.role & Bits(PersonRole::POOR_HEALTH)) {
        if (RandomBelow(ds, static_cast<std::uint16_t>(level * 10 + 24)) == 2)
          person.role &=
              static_cast<std::uint8_t>(~Bits(PersonRole::POOR_HEALTH));
        else
          opeu_sub_byte(static_cast<std::uint16_t>(10 - person.byte_17 / 10),
                        &person.health);
        if (person.health == 0) {
          if (person.role & Bits(PersonRole::DEPUTY))
            person.health = 1;
          else if ((person.role & Bits(PersonRole::ROLE_MASK)) == GENERAL_ROLE)
            person.role |= Bits(PersonRole::DEPUTY);
          else
            opeu_commander_death(
                ds, event, static_cast<std::uint16_t>(sp - DEATH_BP_BELOW_SP),
                Near(data, person));
        }
      }
      std::uint8_t amount{ 2 };
      if (person.assignment & Bits(Assignment::DIVISION))
        amount = 1;
      else if (opeu_get_record_counter(
                   reinterpret_cast<std::uint8_t*>(&person)))
        amount = 3;
      opeu_add_byte(amount, person.byte_17, &person.health);
    }
  }
}

uint16_t svc_evwound(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  switch (id) {
    case DELAY:
      opeu_delay_units(ds, svc_waitkey,
                       static_cast<std::uint8_t>(Arg(args, count, 0)));
      break;
    case CONTEXT:
      opeu_set_turn_context(ds, svc_sound,
                            static_cast<std::uint8_t>(Arg(args, count, 0)));
      break;
    case FINISH:
      opeu_finish_side_initialization(
          ds, svc_sound, static_cast<std::uint8_t>(Arg(args, count, 0)));
      break;
    case SHOW:
      port_daily_show(ds, Arg(args, count, 0), Arg(args, count, 1),
                      Arg(args, count, 2), Arg(args, count, 3));
      break;
    case MESSAGE:
      opeu_control_message(ds, (const DialogHost*)port_dialogs_host(),
                           Arg(args, count, 0), Arg(args, count, 1));
      break;
    default: port_unhandled("evwound", id, count);
  }
  return 0;
}

auto port_routine_opeu_death_present(std::uint8_t* ds, std::uint16_t const*,
                                     unsigned) -> std::uint32_t {
  opeu_death_present(ds, svc_evwound);
  return 0;
}

auto port_routine_opeu_wound_event(std::uint8_t* ds, std::uint16_t const*,
                                   unsigned) -> std::uint32_t {
  WithStack(PORT_FRAME,
            [&](std::uint16_t sp) { opeu_wound_event(ds, svc_evwound, sp); });
  return 0;
}
