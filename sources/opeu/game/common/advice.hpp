// The staff advice (core/adv*): the far advice texts, the five advice buffers and the services an advice check calls.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/memory.hpp"
#include "core/textout.hpp"
#include "units/unitname.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <utility>

namespace opeu::game::detail::advice {

// The topics of the staff advice menu; each writes its own advice buffer.
enum class AdviceTopic : std::uint8_t {
  TACTICS     = 0,
  REGIMENTS   = 1,
  ENEMY       = 2,
  TIDE_OF_WAR = 3,
  HINTS       = 4,
};

// The paragraph of the far advice texts (data/fadvice); the general tips' far pointers point into it too.
inline constexpr std::uint16_t ADVICE_SEGMENT{ 0x3ad9 };

// The advice texts by their offset in the far advice segment (data/fadvice, in order).
enum class AdviceText : std::uint16_t {
  DEFEND_OBJECTIVE        = 0x0000, // "Our priority is the defense of %s."
  TAKE_OBJECTIVE          = 0x0025, // "This is our chance to take %s."
  APPOINT_COMMANDER       = 0x0046, // "... who will take over the Division %d Commander position."
  LEAVE_TO_COMMANDER      = 0x0091, // "You look tired...Why not leave the Division Commander in charge?"
  MOVE_HEADQUARTERS       = 0x00d4, // "It's too dangerous here. We should move Army HQ."
  REORGANIZE              = 0x0107, // "Let's stop everything and reorganize."
  SEND_HELP               = 0x012f, // "We should send help to %s."
  SEND_TO_FRONT           = 0x014c, // "Let's send %s to the frontlines."
  PULL_BACK               = 0x016f, // "Let's pull back behind %s."
  REQUEST_SUPPLIES        = 0x018c, // "We're down to our last supplies. We should request more."
  REINFORCE_DIVISION      = 0x01c7, // "We need to send reinforcements to Division %d, ..."
  PLANT_MINES             = 0x0210, // "We should plant land mines ..."
  ASK_AIR_FORCE           = 0x0254, // "Let's ask the Air Force for help."
  CALL_SPECIAL_FORCES     = 0x0278, // "We should call in the Special Forces Commander."
  PARATROOP_ATTACK        = 0x02aa, // "I suggest a paratrooper attack at the enemy's rear flank."
  SOLIDIFY_DEFENSES       = 0x02e6, // "I think we should solidify our defenses at %s."
  RUSH_OBJECTIVE          = 0x0317, // "We should just rush right in and take %s."
  UNIT_READY_SOON         = 0x0343, // "%s will be ready for battle shortly."
  OPERATION_FINISHING     = 0x036a, // "%s's operations will be finished shortly."
  UNIT_SHORT_OF           = 0x0396, // "%s is short of %s."
  COMMANDER_WOUNDED       = 0x03ab, // "Division %d Commander, %s, is seriously wounded."
  COMMANDER_FATIGUED      = 0x03de, // "%s, Division %d Commander, and his men are fatigued."
  DAMAGED_ARMS            = 0x043d, // "%s has quite a lot of damaged arms."
  COMMUNICATIONS_OUT      = 0x0463, // "Communications with %s are still out."
  UNIT_EXHAUSTED          = 0x048b, // "%s is exhausted."
  UNIT_RESTED             = 0x049e, // "It looks like %s has rested enough."
  MORALE_HIGH             = 0x04c4, // "The morale in %s is very high."
  MORALE_DROPPING         = 0x04e5, // "The morale in %s is dropping dramatically."
  NOTHING_TO_REPORT       = 0x0512, // "There is nothing right now."
  ENEMY_MINE_FIELD        = 0x0530, // "It looks like the enemy is making a mine field."
  ENEMY_BOMBARDMENT       = 0x0562, // "The enemy fleets are preparing to bombard us."
  ENEMY_COMMANDER_UNFIT   = 0x0592, // "The Commander of enemy Division %d, is not fit for battle."
  ENEMY_AIR_RAIDS         = 0x05cf, // "Be careful of enemy air raids."
  ENEMY_PARATROOPS        = 0x05f0, // "The enemy's paratroop operation has started."
  ENEMY_CARGO_PLANES      = 0x061f, // "It looks like the enemy cargo plane have started to load."
  ENEMY_NO_ATTACK_PLANS   = 0x065b, // "It doesn't look like the enemy has any plans to attack just now."
  ENEMY_ATTACK_SOON       = 0x069e, // "The enemy %s attack will come shortly."
  ENEMY_RESTING           = 0x06c7, // "The enemy %s is resting."
  ENEMY_PLOTS_TO_TAKE     = 0x06e2, // "The enemy is plotting to take %s."
  ENEMY_STRENGTHENS       = 0x0706, // "The enemy is strengthening its defenses in %s."
  ENEMY_RECON_ACTIVE      = 0x0737, // "Enemy recon teams are really starting to get active."
  ENEMY_KNOWS_PLANS       = 0x076e, // "It would seem that the enemy knows of our plans."
  PLENTY_OF_MATERIALS     = 0x07a1, // "There are plenty of materials in %s."
  ENEMY_SLOWING           = 0x07c8, // "Enemy %s movement seems to be slowing down."
  ENEMY_ACTIVE            = 0x07f6, // "Enemy %s has been really active recently."
  ENEMY_COMMANDER_FOUND   = 0x0822, // "It seems that Commander %s, is in Division %d."
  NEED_RECON              = 0x0853, // "We don't have enough information to go on. Send in some recon teams."
  OBJECTIVE_SAFE          = 0x089a, // "%s is well isolated from the enemy's territories."
  ABANDON_OBJECTIVE       = 0x08ce, // "We have no choice but to abandon %s."
  FORCES_ISOLATED         = 0x08f5, // "Our forces are isolated within the enemy's territories."
  WAIT_FOR_RECOVERY       = 0x0960, // "We will wait now for our recovery."
  GO_ON_OFFENSIVE         = 0x0985, // "It's just the right time to go on the offensive."
  WAIT_IT_OUT             = 0x09b8, // "We will just have to wait it out."
  WAIT_FOR_REINFORCEMENTS = 0x09dc, // "We should wait for our reinforcements in Division %d to arrive."
  SITUATION_WORST         = 0x0a1e, // "The situation couldn't be worse."
  OVERWHELMING_ENEMY      = 0x0a41, // "We are overwhelming the enemy."
  ENEMY_POWERFUL          = 0x0a62, // "The enemy is very powerful."
  ENEMY_EQUAL             = 0x0a80, // "The enemy is just as strong as we are."
  ENEMY_RULES_SKIES       = 0x0aa9, // "The enemy rules the skies."
  WE_RULE_SKIES           = 0x0ac6, // "We rule the skies."
  NO_ADVANCES             = 0x0adb, // "There have been no advances."
  HINT_AIR_SUPPORT        = 0x0afa, // "Without control of the skies, we shouldn't hope for too much air support."
  HINT_CARPET_BOMBING     = 0x0b46, // "Carpet bombing is usually used to weaken cities."
  HINT_AIR_GUNFIRE        = 0x0b79, // "Gunfire from the air is especially effective against ground troops."
  HINT_RESERVE_DIVISION   = 0x0bbf, // "Division %d will arrive later with reinforcements."
  HINT_DEPUTY             = 0x0bf4, // "Until a new Commander for Division %d is appointed, a Deputy will take over."
  HINT_STAFF_DISAGREE     = 0x0c43, // "The Chief of Intelligence and Air Commander do not agree very often."
  HINT_ARDENNES_WINTER    = 0x0c8a, // "The winters in Ardennes are horrible."
  HINT_HURRICANE_SEASON   = 0x0cb2, // "It's hurricane season in Normandy now."
  HINT_BUILD_BRIDGE       = 0x0cdb, // "Let's build a bridge and start moving our divisions."
  HINT_DESTROY_BRIDGES    = 0x0d12, // "If we knock out the bridges, it will slow the enemy's advance ..."
};

// The far advice texts.
[[nodiscard]] inline auto AdviceTexts(Game const& game) -> std::span<std::uint8_t const> {
  return FarBytes(game, ADVICE_SEGMENT, 0);
}

// The near pointer of a topic's advice buffer.
[[nodiscard]] inline auto AdviceBuffer(Game& game, AdviceTopic topic) -> std::uint16_t {
  auto& data{ Data(game) };
  return Near(data, data.advice_messages.at(std::to_underlying(topic)));
}

// Shows a text in a control message in the person of the staff officer giving the advice (context_record2).
inline void ShowAdvice(Game& game, Text text) {
  opeu_control_message(game, text, Data(game).context_record2);
}

// The objective level (opeu_objective_level) from which the officers count an objective as a city.
inline constexpr std::uint8_t CITY_LEVEL{ 4 };

// The bytes of an advice text an advice check formats (the longest is 107 bytes) and of a unit name.
inline constexpr std::uint16_t ADVICE_TEXT_BYTES{ 0x80 };
inline constexpr std::uint16_t UNIT_NAME_BYTES{ 0x40 };

// An advice check's context: the far advice texts and the topic whose buffer it writes.
struct Advisor {
  Game&                         game;
  std::span<std::uint8_t const> far_text;
  AdviceTopic                   topic;

  // The topic's advice buffer.
  [[nodiscard]] auto Buffer() const -> std::uint16_t {
    return AdviceBuffer(game, topic);
  }
  [[nodiscard]] auto Random(std::uint16_t bound) const -> std::uint16_t {
    return game.State().RandomBounded(bound);
  }
  // Copies a far advice text into a text.
  void Copy(TextTarget destination, AdviceText text) const {
    opeu_string_copy_far(game, destination, far_text.subspan(std::to_underlying(text)));
  }
  // Formats into the topic's buffer.
  template <typename... _Words>
  void Format(Text format, _Words... words) const {
    opeu_request_format(game, Buffer(), std::array<FormatArgument, 1 + sizeof...(_Words)>{ format, words... });
  }
  // Shows the topic's buffer.
  void Show() const {
    ShowAdvice(game, Buffer());
  }
  // Gives the advice text as it stands; returns 1, the check's "advice given".
  [[nodiscard]] auto Say(AdviceText text) const -> std::uint16_t {
    Copy(Buffer(), text);
    Show();
    return 1;
  }
  // Gives the advice text formatted with the words; the formatting service reads the text from the data segment.
  // Returns 1.
  template <typename... _Words>
  [[nodiscard]] auto SayFormatted(AdviceText text, _Words... words) const -> std::uint16_t {
    TextBuffer<ADVICE_TEXT_BYTES> format;
    Copy(format, text);
    Format(format, words...);
    Show();
    return 1;
  }
  // Gives the advice text formatted with the unit's name and the further words; returns 1.
  template <typename... _Words>
  [[nodiscard]] auto SayAboutUnit(AdviceText text, std::uint16_t unit, _Words... words) const -> std::uint16_t {
    TextBuffer<UNIT_NAME_BYTES> name;
    return SayFormatted(text, opeu_unit_name(game, unit, name), words...);
  }
};
} // namespace opeu::game::detail::advice

namespace opeu::game {
using detail::advice::ADVICE_SEGMENT;
using detail::advice::ADVICE_TEXT_BYTES;
using detail::advice::AdviceBuffer;
using detail::advice::AdviceText;
using detail::advice::AdviceTexts;
using detail::advice::AdviceTopic;
using detail::advice::Advisor;
using detail::advice::ShowAdvice;
using detail::advice::UNIT_NAME_BYTES;
using detail::advice::CITY_LEVEL;
} // namespace opeu::game
