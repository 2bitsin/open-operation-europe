// The game's zero-terminated strings: copied, appended and measured.
#include "core/strings.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "common/text.hpp"

#include <string>

// Copies source with its terminator to destination; returns destination.
TextTarget opeu_string_copy(Game& game, TextTarget destination, Text source) {
  auto& data{ Data(game) };
  StoreText(data, destination, std::string{ source.View(data) });
  return destination;
}

// Appends source to the string at destination; returns destination.
TextTarget opeu_string_append(Game& game, TextTarget destination, Text source) {
  auto& data{ Data(game) };
  StoreText(data, destination, std::string{ destination.Written().View(data) } + std::string{ source.View(data) });
  return destination;
}

uint16_t opeu_string_length(Game& game, Text string) {
  return static_cast<uint16_t>(string.View(Data(game)).size());
}
