// The game calendar: leap years and month lengths.
#pragma once

#include "types.hpp"

uint16_t opeu_is_leap_year(uint16_t year);
uint8_t  opeu_days_in_month(uint8_t year_since_1940, uint8_t month);
