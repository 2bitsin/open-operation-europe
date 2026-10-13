// A general's chance of being wounded or killed.
//
// Generals in battle (assignment bit 2) and generals posted as 2 or 3 roll for a wound or for death; a general posted
// as 1 on a side the player controls rolls for a silent wound or death first. A wound shown to a human side is
// announced with the record temporarily replaced by its deputy and then restored from a copy; a death is announced
// through the daily show with opeu_blast_pause.
#include "units/genwound.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/numeric.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "core/deputy.hpp"
#include "core/dialogs.hpp"
#include "core/personfl.hpp"
#include "glue.hpp"
#include "io/keyboard.hpp"
#include "io/sound.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the module calls through its event.
enum Service : std::uint16_t {
  PERSON_LACKS_SIDE0_FLAG    = 0, // (person) -> side
  REQUEST_FORMAT             = 1, // (buffer, format, person, text)
  INITIALIZE_DEPUTY          = 2, // (person)
  CONTROL_MESSAGE            = 3, // (text, person)
  WAIT_CONFIRM               = 4, // (value)
  SET_TURN_CONTEXT           = 5, // (value)
  FINISH_SIDE_INITIALIZATION = 6, // (value)
  DAILY_SHOW                 = 7, // (id, side, presenter offset, presenter segment)
};

enum class Outcome : std::uint8_t {
  NONE,
  WOUNDED,
  KILLED,
};

constexpr std::uint8_t  KILLED_FLAG{ 0x80 }; // Person::flags: the general is dead
constexpr std::uint8_t  IN_BATTLE{ 0x04 };   // Person::assignment: the general is fighting
constexpr std::uint16_t DEATH_SHOW{ 300 };   // the daily show that announces a death
constexpr std::uint16_t HUMAN{ 1 };          // side_status / side_control of a player side
// The far address by which the dialogs service finds the presenter opeu_blast_pause.
constexpr std::uint16_t BLAST_PAUSE_OFFSET{ 0x0866 };
constexpr std::uint16_t BLAST_PAUSE_SEGMENT{ 0x17c4 };

constexpr std::uint16_t WOUNDED_TEXT{ offsetof(DataSegment, general_wounded) };
constexpr std::uint16_t KILLED_TEXT{ offsetof(DataSegment, general_killed) };
constexpr std::uint16_t CASUALTY_FORMAT{ offsetof(DataSegment, general_casualty_format) };

// The message buffer the services format and show.
constexpr std::uint16_t MESSAGE_BYTES{ 0x88 };
} // namespace

void opeu_general_casualty(uint8_t ds[65536], HostEvent event, uint16_t person) {
  auto&               data{ Data(ds) };
  auto&               general{ Record<Person>(data, person) };
  ServiceLocals const message{ MESSAGE_BYTES };
  auto const          buffer{ message.At(0) };
  auto const          wounded{ [&] { return (general.flags & Bits(PersonRole::POOR_HEALTH)) != 0; } };
  auto const          posting{ [&] { return general.flags & Bits(PersonRole::ROLE_MASK); } };
  auto const          wound{ [&] {
    opeu_sub_byte(RandomBelow(ds, 20) + 10, &general.health);
    general.flags |= Bits(PersonRole::POOR_HEALTH);
  } };

  if (general.flags & KILLED_FLAG) return;
  auto       outcome{ Outcome::NONE };
  auto const side{ static_cast<std::uint8_t>(Call(ds, event, PERSON_LACKS_SIDE0_FLAG, person)) };
  if (general.assignment & IN_BATTLE) {
    if (!wounded() && RandomBelow(ds, 20) == 2)
      outcome = Outcome::WOUNDED;
    else if (RandomBelow(ds, 40) == 4)
      outcome = Outcome::KILLED;
  }
  if (posting() == 2 || posting() == 3) {
    if (!wounded() && RandomBelow(ds, 30) == 2)
      outcome = Outcome::WOUNDED;
    else if (RandomBelow(ds, 50) == 4)
      outcome = Outcome::KILLED;
  }
  if (posting() == 1 && data.side_control[side] == HUMAN) {
    if (!wounded() && RandomBelow(ds, 40) == 2) {
      wound();
      return;
    }
    if (RandomBelow(ds, 60) == 4) {
      general.flags |= KILLED_FLAG;
      return;
    }
  }
  if (outcome == Outcome::WOUNDED) {
    if (data.side_status[side] == HUMAN) {
      auto const saved{ general };
      Call(ds, event, REQUEST_FORMAT, buffer, CASUALTY_FORMAT, person, WOUNDED_TEXT);
      Call(ds, event, INITIALIZE_DEPUTY, person);
      Call(ds, event, CONTROL_MESSAGE, buffer, person);
      Call(ds, event, WAIT_CONFIRM, 1);
      general = saved;
    }
    wound();
  } else if (outcome == Outcome::KILLED) {
    Call(ds, event, REQUEST_FORMAT, buffer, CASUALTY_FORMAT, person, KILLED_TEXT);
    Call(ds, event, INITIALIZE_DEPUTY, person);
    if (data.side_status[side] == HUMAN) {
      Call(ds, event, SET_TURN_CONTEXT, 0x10);
      Call(ds, event, FINISH_SIDE_INITIALIZATION, 0);
      Call(ds, event, DAILY_SHOW, DEATH_SHOW, side, BLAST_PAUSE_OFFSET, BLAST_PAUSE_SEGMENT);
      Call(ds, event, CONTROL_MESSAGE, buffer, person);
      Call(ds, event, WAIT_CONFIRM, 1);
    }
  }
}

// The services of opeu_general_casualty; the daily show reaches the presenter opeu_blast_pause by its far address
// through the dialogs host.
uint16_t svc_genwound(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case PERSON_LACKS_SIDE0_FLAG:    r = opeu_person_lacks_side0_flag(ds, Arg(args, count, 0)); break;
  case REQUEST_FORMAT:             opeu_request_format(ds, { args, count }); break;
  case INITIALIZE_DEPUTY:          opeu_initialize_deputy(ds, Arg(args, count, 0)); break;
  case CONTROL_MESSAGE:            opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case WAIT_CONFIRM:               opeu_wait_confirm(ds, svc_keyboard, (int16_t)Arg(args, count, 0)); break;
  case SET_TURN_CONTEXT:           opeu_set_turn_context(ds, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case FINISH_SIDE_INITIALIZATION: opeu_finish_side_initialization(ds, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case DAILY_SHOW: opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint32_t)Arg(args, count, 3) << 16 | Arg(args, count, 2)); break;
  default:         port_unhandled("genwound", id, count);
  }
  return r;
}
