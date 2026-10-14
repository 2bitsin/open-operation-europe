#pragma once
// The game's texts as UTF-8 messages by id (texts/messages.json, docs/asset-pack.md "Texts"): every text the shipped
// files hold, which is SCEDATA's: the scenario descriptions, the victory conditions and the set-ups' names. The ids:
// scenario/<n>/description, scenario/<n>/condition/<side>, setup/<n>/person/<nn>/name, setup/<n>/objective/<nn>/name.
// The code table turns each message back into the bytes SCEDATA holds.

#include "opeu/assets/bytes.hpp"
#include "opeu/assets/code-table.hpp"
#include "opeu/assets/pack-error.hpp"
#include "opeu/assets/scenario.hpp"

#include <map>
#include <string>
#include <string_view>

namespace opeu::assets::detail::messages {

inline constexpr auto MESSAGES{ std::string_view{ "texts/messages.json" } };

// Id -> text: UTF-8 in a message, KOEI's code in the texts a file holds.
using Messages = std::map<std::string, std::string, std::less<>>;

// The texts of the scenario file by id, as its bytes; an empty name is no text.
[[nodiscard]] auto ScenarioTexts(ScenarioFile const& file) -> Messages;

// Texts decoded into messages.
[[nodiscard]] auto MessagesOf(Messages const& texts, CodeTable const& table) -> Messages;

[[nodiscard]] auto MessagesText(Messages const& messages) -> std::string;

[[nodiscard]] auto ReadMessages(Bytes bytes) -> Packed<Messages>;

}

namespace opeu::assets {
using detail::messages::Messages;
using detail::messages::MESSAGES;
using detail::messages::MessagesOf;
using detail::messages::MessagesText;
using detail::messages::ReadMessages;
using detail::messages::ScenarioTexts;
}
