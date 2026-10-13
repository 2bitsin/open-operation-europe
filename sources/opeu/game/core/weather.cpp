// The hourly weather change.
#include "core/weather.hpp"
#include "common/game.hpp"
#include "core/numeric.hpp"
#include "data-segment.hpp"
#include "data/fg3a64.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {

constexpr auto WEATHER_KINDS{ 6u };

} // namespace

// One hour in four the weather is rolled again: a 1..100 roll is spent against the chances of each next weather in
// the row for the month's climate and the present weather; the weather that uses up the roll comes, and when it
// differs from the present one its terrain ratings replace the current ones.
void opeu_weather_update(Game& game) {
  auto& data{ Data(game) };
  if (game.State().RandomBounded(4) != 0) return;
  auto roll{ static_cast<uint16_t>(game.State().RandomBounded(100) + 1) };
  for (uint16_t next = 0; next < WEATHER_KINDS; next++) {
    auto const row{ static_cast<uint16_t>((data.month_climate[data.month] * WEATHER_KINDS + data.weather) * WEATHER_KINDS) };
    roll = SubWord(roll, data.weather_odds[row + next]);
    if (roll == 0) {
      if (data.weather != next) {
        data.weather = static_cast<uint8_t>(next);
        for (uint16_t n = 0; n < 55; n++) data.terrain_ratings[n] = opeu_weather_terrain_ratings[next][n];
      }
      break;
    }
  }
}
