// The game calendar: leap years and month lengths.
#include "core/calendar.hpp"
#include "types.hpp"

#include <array>

namespace {

constexpr std::array<std::uint8_t, 13> DAYS{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
constexpr std::uint16_t                FIRST_YEAR{ 1940 };
constexpr std::uint8_t                 FEBRUARY{ 2 };
} // namespace

// 1 when the Gregorian year is a leap year, else 0.
uint16_t opeu_is_leap_year(uint16_t year) {
  std::uint16_t result = year % 4 == 0;
  if (year % 100 == 0) result = result - (year % 400 != 0);
  return result;
}

// The days of a month (1..12) of the year 1940 + year_since_1940; month 0 has none.
uint8_t opeu_days_in_month(uint8_t year_since_1940, uint8_t month) {
  std::uint8_t result{ DAYS[month] };
  if (month == FEBRUARY)
    if (opeu_is_leap_year(static_cast<std::uint16_t>(year_since_1940 + FIRST_YEAR))) ++result;
  return result;
}
