#include "opeu/assets/scenario.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace opeu::assets::detail::scenario {

namespace {

// Where the records sit in a set-up (the set-up starts with the byte 'E' and ends with 'T').
constexpr auto HEADER{ 0x01uz };
constexpr auto SIDES{ 0x1D8uz };
constexpr auto SIDE_BYTES{ 65uz };
// Where a table of records sits in a set-up: its offset, the size of a record and the number of records.
struct TableAt {
  std::size_t at;
  std::size_t size;
  std::size_t count;
};

constexpr auto PEOPLE{
  TableAt{ .at = 0x25A, .size = 31, .count = 40 }
};
constexpr auto ELEMENTS{
  TableAt{ .at = 0x7BA, .size = 13, .count = 256 }
};
constexpr auto UNITS{
  TableAt{ .at = 0x14BA, .size = 22, .count = 64 }
};
constexpr auto OBJECTIVES{
  TableAt{ .at = 0x1A3A, .size = 68, .count = 21 }
};

// The NUL-terminated text of a fixed-size field.
auto Text(Bytes const field) -> std::string {
  return field | std::views::take_while([](std::byte const b) { return b != std::byte{ 0 }; }) | std::views::transform([](std::byte const b) { return static_cast<char>(b); })
         | std::ranges::to<std::string>();
}

auto Words3(Bytes const bytes, std::size_t const at) -> std::array<unsigned, 3> {
  return { ReadWord(bytes, at), ReadWord(bytes, at + 2), ReadWord(bytes, at + 4) };
}

auto Person(Bytes const r) -> scenario::Person {
  auto person{
    scenario::Person{ .name = Text(r.first(18)), .side = Byte(r, 18), .abilities{}, .portrait = Byte(r, 29) }
  };
  std::ranges::transform(r.subspan(19, person.abilities.size()), person.abilities.begin(), [](std::byte const b) { return std::to_integer<std::uint8_t>(b); });
  return person;
}

auto Element(Bytes const r) -> scenario::Element {
  return { .next = ReadWord(r, 0), .type = Byte(r, 2), .quantity = ReadWord(r, 3), .morale = Byte(r, 9), .training = Byte(r, 10), .experience = Byte(r, 11), .state = Byte(r, 12) };
}

auto Unit(Bytes const r) -> scenario::Unit {
  return { .first_element = ReadWord(r, 0), .x = Byte(r, 2), .y = Byte(r, 3), .supplies = Words3(r, 6), .flags = Byte(r, 21) };
}

auto Objective(Bytes const r) -> scenario::Objective {
  return { .name = Text(r.first(0x15)), .x = Byte(r, 0x15), .y = Byte(r, 0x16), .garrison = Byte(r, 0x23), .points = Byte(r, 0x24), .flags = Byte(r, 0x43) };
}

template <class _Read>
auto Table(Bytes const setup, TableAt const& table, _Read read) {
  return Records(setup.subspan(table.at), table.size, table.count) | std::views::transform(read) | std::ranges::to<std::vector>();
}

auto DecodeSetup(Bytes const s) -> Setup {
  auto setup{
    Setup{ .scenario   = Byte(s, HEADER),
          .year       = static_cast<unsigned>(1940 + Byte(s, HEADER + 1)),
          .month      = Byte(s, HEADER + 2),
          .day        = Byte(s, HEADER + 3),
          .hour       = Byte(s, HEADER + 4),
          .weather    = Byte(s, HEADER + 5),
          .difficulty = Byte(s, HEADER + 8),
          .sides{},
          .people     = Table(s, PEOPLE, Person),
          .elements   = Table(s, ELEMENTS, Element),
          .units      = Table(s, UNITS, Unit),
          .objectives = Table(s, OBJECTIVES, Objective) }
  };
  for (auto const side : Indices(2)) {
    auto const at{ SIDES + (side * SIDE_BYTES) };
    setup.sides.at(side) = Side{ .stock = Words3(s, at), .income = Words3(s, at + 6) };
  }
  return setup;
}

}

ScenarioFile::ScenarioFile(Descriptions descriptions, Conditions conditions, Setups setups)
    : _descriptions{ std::move(descriptions) }, _conditions{ std::move(conditions) }, _setups{ std::move(setups) } { }

auto ScenarioFile::Description(std::size_t const n) const -> std::string const& {
  return _descriptions.at(n);
}

auto ScenarioFile::Condition(std::size_t const n, std::size_t const side) const -> std::string const& {
  return _conditions.at(n).at(side);
}

auto ScenarioFile::SetupOf(std::size_t const n) const -> Setup const& {
  return _setups.at(n);
}

auto DecodeScenarioFile(Bytes const bytes) -> Decoded<ScenarioFile> {
  if (bytes.size() != FILE_BYTES) return std::unexpected{ DecodeError::BAD_SIZE };
  auto           descriptions{ ScenarioFile::Descriptions{} };
  auto           conditions{ ScenarioFile::Conditions{} };
  auto           setups{ ScenarioFile::Setups{} };
  constexpr auto HALF{ TEXT_BYTES / 2 };
  for (auto const n : Indices(SCENARIOS)) {
    descriptions.at(n) = Text(bytes.subspan(n * TEXT_BYTES, TEXT_BYTES));
    for (auto const side : Indices(2)) {
      conditions.at(n).at(side) = Text(bytes.subspan(((SCENARIOS + n) * TEXT_BYTES) + (side * HALF), HALF));
    }
  }
  for (auto const n : Indices(SETUPS)) setups.at(n) = DecodeSetup(bytes.subspan(SETUPS_AT + (n * SETUP_BYTES), SETUP_BYTES));
  return ScenarioFile{ std::move(descriptions), std::move(conditions), std::move(setups) };
}

}
