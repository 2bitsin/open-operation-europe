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

#include "common/game.hpp"
#include "core/deputy.hpp"
#include "core/dialogs.hpp"
#include "core/personfl.hpp"
#include "io/keyboard.hpp"
#include "io/sound.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

enum class Outcome : std::uint8_t {
  NONE,
  WOUNDED,
  KILLED,
};

constexpr std::uint8_t  KILLED_FLAG{ 0x80 }; // Person::flags: the general is dead
constexpr std::uint8_t  IN_BATTLE{ 0x04 };   // Person::assignment: the general is fighting
constexpr std::uint16_t DEATH_SHOW{ 300 };   // the daily show that announces a death
// The far address by which the dialogs service finds the presenter opeu_blast_pause.
constexpr std::uint16_t BLAST_PAUSE_OFFSET{ 0x0866 };
constexpr std::uint16_t BLAST_PAUSE_SEGMENT{ 0x17c4 };

constexpr std::uint16_t WOUNDED_TEXT{ offsetof(DataSegment, general_wounded) };
constexpr std::uint16_t KILLED_TEXT{ offsetof(DataSegment, general_killed) };
constexpr std::uint16_t CASUALTY_FORMAT{ offsetof(DataSegment, general_casualty_format) };

// The message buffer the services format and show.
constexpr std::uint16_t MESSAGE_BYTES{ 0x88 };
} // namespace

void opeu_general_casualty(Game& game, uint16_t person) {
  auto&               data{ Data(game) };
  auto&               general{ Record<Person>(data, person) };
  ServiceLocals const message{ MESSAGE_BYTES };
  auto const          buffer{ message.At(0) };
  auto const          wounded{ [&] { return (general.flags & Bits(PersonRole::POOR_HEALTH)) != 0; } };
  auto const          posting{ [&] { return general.flags & Bits(PersonRole::ROLE_MASK); } };
  auto const          wound{ [&] {
    opeu_sub_byte(RandomBelow(game, 20) + 10, &general.health);
    general.flags |= Bits(PersonRole::POOR_HEALTH);
  } };

  if (general.flags & KILLED_FLAG) return;
  auto       outcome{ Outcome::NONE };
  auto const side{ static_cast<std::uint8_t>(opeu_person_lacks_side0_flag(game, person)) };
  if (general.assignment & IN_BATTLE) {
    if (!wounded() && RandomBelow(game, 20) == 2)
      outcome = Outcome::WOUNDED;
    else if (RandomBelow(game, 40) == 4)
      outcome = Outcome::KILLED;
  }
  if (posting() == 2 || posting() == 3) {
    if (!wounded() && RandomBelow(game, 30) == 2)
      outcome = Outcome::WOUNDED;
    else if (RandomBelow(game, 50) == 4)
      outcome = Outcome::KILLED;
  }
  if (posting() == 1 && data.side_control[side] == SIDE_PLAYED) {
    if (!wounded() && RandomBelow(game, 40) == 2) {
      wound();
      return;
    }
    if (RandomBelow(game, 60) == 4) {
      general.flags |= KILLED_FLAG;
      return;
    }
  }
  if (outcome == Outcome::WOUNDED) {
    if (data.side_status[side] == SIDE_PLAYED) {
      auto const saved{ general };
      opeu_request_format(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(CASUALTY_FORMAT), static_cast<std::uint16_t>(person),
                                                              static_cast<std::uint16_t>(WOUNDED_TEXT) });
      opeu_initialize_deputy(game, person);
      opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), buffer, person);
      opeu_wait_confirm(game, svc_keyboard, static_cast<int16_t>(1));
      general = saved;
    }
    wound();
  } else if (outcome == Outcome::KILLED) {
    opeu_request_format(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(CASUALTY_FORMAT), static_cast<std::uint16_t>(person),
                                                            static_cast<std::uint16_t>(KILLED_TEXT) });
    opeu_initialize_deputy(game, person);
    if (data.side_status[side] == SIDE_PLAYED) {
      opeu_set_turn_context(game, svc_sound, static_cast<uint8_t>(0x10));
      opeu_finish_side_initialization(game, svc_sound, static_cast<uint8_t>(0));
      opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), DEATH_SHOW, static_cast<uint8_t>(side), static_cast<uint32_t>(BLAST_PAUSE_SEGMENT) << 16 | BLAST_PAUSE_OFFSET);
      opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), buffer, person);
      opeu_wait_confirm(game, svc_keyboard, static_cast<int16_t>(1));
    }
  }
}
