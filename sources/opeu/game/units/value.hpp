// Category and hour classifiers and the combat value of types, elements and units.
#pragma once

#include "types.hpp"

uint16_t opeu_category_0_to_3(uint8_t code);
uint16_t opeu_category_4_or_5(uint8_t code);
uint16_t opeu_category_6(uint8_t code);
uint16_t opeu_category_7(uint8_t code);
uint16_t opeu_category_0_to_5(uint8_t code);
uint16_t opeu_type_value(const uint8_t* ds, uint16_t type);
uint16_t opeu_element_value(const uint8_t* ds, uint16_t element);
uint16_t opeu_unit_value(const uint8_t* ds, uint16_t unit);
uint16_t opeu_hour_is_dawn(uint8_t code);
uint16_t opeu_hour_is_day(uint8_t code);
uint16_t opeu_hour_is_dusk(uint8_t code);
uint16_t opeu_hour_is_night(uint8_t code);
