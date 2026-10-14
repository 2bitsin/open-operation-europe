#pragma once
// SCEDATA: the six scenario descriptions, the victory conditions of each side, and seven scenario set-ups (the
// starting state of a game: calendar, sides, commanders, groups, elements, units and objectives).

#include "opeu/assets/image.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace opeu::assets::detail::scenario {

inline constexpr auto SCENARIOS{ std::size_t{ 6 } };
inline constexpr auto SETUPS{ std::size_t{ 7 } }; // set-up 0 is an unreachable variant of set-up 1
inline constexpr auto TEXT_BYTES{ std::size_t{ 800 } };
inline constexpr auto SETUP_BYTES{ std::size_t{ 0x1FCF } };
inline constexpr auto SETUPS_AT{ 2 * SCENARIOS * TEXT_BYTES };
inline constexpr auto FILE_BYTES{ SETUPS_AT + (SETUPS * SETUP_BYTES) };

struct Side {
  std::array<unsigned, 3> stock{};
  std::array<unsigned, 3> income{};
};

struct Person {
  std::string                 name;
  std::uint8_t                side{};
  std::array<std::uint8_t, 8> abilities{};
  std::uint8_t                portrait{}; // FACE.DAT record
};

struct Element {
  unsigned     next{}; // the next element of the unit, 0xFFFF for none
  std::uint8_t type{}; // the equipment type, also its ARMS.SIL silhouette
  unsigned     quantity{};
  std::uint8_t morale{};
  std::uint8_t training{};
  std::uint8_t experience{};
  std::uint8_t state{};
};

struct Unit {
  unsigned                first_element{}; // 0xFFFF for none
  std::uint8_t            x{};
  std::uint8_t            y{};
  std::array<unsigned, 3> supplies{};
  std::uint8_t            flags{};
};

struct Objective {
  std::string  name; // empty for an unused slot
  std::uint8_t x{};
  std::uint8_t y{};
  std::uint8_t garrison{};
  std::uint8_t points{};
  std::uint8_t flags{};
};

struct Setup {
  std::uint8_t           scenario{}; // 0..5; bit 7 marks the unreachable variant
  unsigned               year{};
  std::uint8_t           month{};
  std::uint8_t           day{};
  std::uint8_t           hour{};
  std::uint8_t           weather{};
  std::uint8_t           difficulty{};
  std::array<Side, 2>    sides{};
  std::vector<Person>    people;     // 40, 20 per side
  std::vector<Element>   elements;   // 256
  std::vector<Unit>      units;      // 64
  std::vector<Objective> objectives; // 21
};

class ScenarioFile {
public:
  using Descriptions = std::array<std::string, SCENARIOS>;
  using Conditions   = std::array<std::array<std::string, 2>, SCENARIOS>;
  using Setups       = std::array<Setup, SETUPS>;

  ScenarioFile(Descriptions descriptions, Conditions conditions, Setups setups);

  // The scenario-selection text of scenario n (0..5).
  [[nodiscard]] auto Description(std::size_t n) const -> std::string const&;
  // What side 0 (Axis) or 1 (Allies) must achieve in scenario n.
  [[nodiscard]] auto Condition(std::size_t n, std::size_t side) const -> std::string const&;
  // Set-up n: 1..6 start scenarios 0..5.
  [[nodiscard]] auto SetupOf(std::size_t n) const -> Setup const&;

private:
  Descriptions _descriptions;
  Conditions   _conditions;
  Setups       _setups;
};

[[nodiscard]] auto DecodeScenarioFile(Bytes bytes) -> Decoded<ScenarioFile>;

}

namespace opeu::assets {
using detail::scenario::DecodeScenarioFile;
using detail::scenario::ScenarioFile;
using detail::scenario::Setup;
}
