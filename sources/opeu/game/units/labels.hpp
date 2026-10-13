// Names of equipment types, items and elements, and map-location labels.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_type_name(Game& game, uint16_t type, uint16_t buffer);
uint16_t opeu_item_name(Game& game, uint16_t item, uint16_t buffer);
uint16_t opeu_element_name(Game& game, uint16_t element, uint16_t buffer);
uint16_t opeu_location_name(Game& game, HostEvent event, uint8_t x, uint8_t y, uint16_t buffer);

// The services the module calls out to.
uint16_t svc_labels(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
