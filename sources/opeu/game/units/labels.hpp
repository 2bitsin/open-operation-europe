// Names of equipment types, items and elements, and map-location labels.
#pragma once

#include "common/events.hpp"
#include "common/text.hpp"
#include "types.hpp"

TextTarget opeu_type_name(Game& game, uint16_t type, TextTarget buffer);
TextTarget opeu_item_name(Game& game, uint16_t item, TextTarget buffer);
TextTarget opeu_element_name(Game& game, uint16_t element, TextTarget buffer);
TextTarget opeu_location_name(Game& game, uint8_t x, uint8_t y, TextTarget buffer);
