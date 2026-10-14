// The game's zero-terminated strings: copied, appended and measured.
#pragma once

#include "common/events.hpp"
#include "common/text.hpp"
#include "types.hpp"

TextTarget opeu_string_copy(Game& game, TextTarget destination, Text source);
TextTarget opeu_string_append(Game& game, TextTarget destination, Text source);
uint16_t   opeu_string_length(Game& game, Text string);
