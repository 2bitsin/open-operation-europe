#include "opeu/assets/scenario-table.hpp"

#include "opeu/assets/table-json.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <utility>
#include <vector>

namespace opeu::assets::detail::scenario_table {

namespace {

using namespace table_json;
using scenario::Element;
using scenario::Objective;
using scenario::Person;
using scenario::Side;
using scenario::Unit;

// An array of a set-up's records: its key and how many records it holds.
struct Records {
  std::string_view key;
  std::size_t      count;
};

constexpr auto PEOPLE{
  Records{ .key = "people", .count = 40 }
};
constexpr auto ELEMENTS{
  Records{ .key = "elements", .count = 256 }
};
constexpr auto UNITS{
  Records{ .key = "units", .count = 64 }
};
constexpr auto OBJECTIVES{
  Records{ .key = "objectives", .count = 21 }
};

// Writing: each record as an object of its fields.

auto SideJson(Side const& side) -> Json {
  auto record      = Json::object();
  record["stock"]  = side.stock;
  record["income"] = side.income;
  return record;
}

auto PersonJson(Person const& person) -> Json {
  auto record         = TextMember("name", person.name);
  record["side"]      = person.side;
  record["abilities"] = person.abilities;
  record["portrait"]  = person.portrait;
  return record;
}

auto ElementJson(Element const& element) -> Json {
  auto record          = Json::object();
  record["next"]       = element.next;
  record["type"]       = element.type;
  record["quantity"]   = element.quantity;
  record["morale"]     = element.morale;
  record["training"]   = element.training;
  record["experience"] = element.experience;
  record["state"]      = element.state;
  return record;
}

auto UnitJson(Unit const& unit) -> Json {
  auto record             = Json::object();
  record["first_element"] = unit.first_element;
  record["x"]             = unit.x;
  record["y"]             = unit.y;
  record["supplies"]      = unit.supplies;
  record["flags"]         = unit.flags;
  return record;
}

auto ObjectiveJson(Objective const& objective) -> Json {
  auto record        = TextMember("name", objective.name);
  record["x"]        = objective.x;
  record["y"]        = objective.y;
  record["garrison"] = objective.garrison;
  record["points"]   = objective.points;
  record["flags"]    = objective.flags;
  return record;
}

template <class _Records, class _Write>
auto Each(_Records const& records, _Write write) -> Json {
  return Json(records | std::views::transform(write) | std::ranges::to<std::vector<Json>>());
}

auto SetupJson(Setup const& setup) -> Json {
  auto record          = Json::object();
  record["scenario"]   = setup.scenario;
  record["year"]       = setup.year;
  record["month"]      = setup.month;
  record["day"]        = setup.day;
  record["hour"]       = setup.hour;
  record["weather"]    = setup.weather;
  record["difficulty"] = setup.difficulty;
  record["sides"]      = Each(setup.sides, SideJson);
  record["people"]     = Each(setup.people, PersonJson);
  record["elements"]   = Each(setup.elements, ElementJson);
  record["units"]      = Each(setup.units, UnitJson);
  record["objectives"] = Each(setup.objectives, ObjectiveJson);
  return record;
}

auto ScenarioJson(ScenarioFile const& file, std::size_t const n) -> Json {
  auto record           = Json::object();
  record["description"] = file.Description(n);
  record["conditions"]  = std::array{ file.Condition(n, 0), file.Condition(n, 1) };
  return record;
}

// Reading: each record from its fields, refused when one is missing or out of range.

auto SideOf(Json const& record) -> Side {
  return Side{ .stock = Numbers<3>(record, "stock", 0xFFFF), .income = Numbers<3>(record, "income", 0xFFFF) };
}

auto PersonOf(Json const& record) -> Person {
  auto person{
    Person{ .name = TextOf(record, "name"), .side = ByteField(record, "side"), .abilities{}, .portrait = ByteField(record, "portrait") }
  };
  std::ranges::transform(Numbers<8>(record, "abilities", 0xFF), person.abilities.begin(), [](std::uint32_t const ability) { return static_cast<std::uint8_t>(ability); });
  return person;
}

auto ElementOf(Json const& record) -> Element {
  return Element{ .next       = WordField(record, "next"),
                  .type       = ByteField(record, "type"),
                  .quantity   = WordField(record, "quantity"),
                  .morale     = ByteField(record, "morale"),
                  .training   = ByteField(record, "training"),
                  .experience = ByteField(record, "experience"),
                  .state      = ByteField(record, "state") };
}

auto UnitOf(Json const& record) -> Unit {
  return Unit{ .first_element = WordField(record, "first_element"),
               .x             = ByteField(record, "x"),
               .y             = ByteField(record, "y"),
               .supplies      = Numbers<3>(record, "supplies", 0xFFFF),
               .flags         = ByteField(record, "flags") };
}

auto ObjectiveOf(Json const& record) -> Objective {
  return Objective{ .name     = TextOf(record, "name"),
                    .x        = ByteField(record, "x"),
                    .y        = ByteField(record, "y"),
                    .garrison = ByteField(record, "garrison"),
                    .points   = ByteField(record, "points"),
                    .flags    = ByteField(record, "flags") };
}

// The records of a set-up's array, each read by `read`.
template <class _Read>
auto Listed(Json const& setup, Records const records, _Read read) {
  return Sized(setup, records.key, records.count) | std::views::transform(read) | std::ranges::to<std::vector>();
}

auto SidesOf(Json const& record) -> std::array<Side, 2> {
  auto sides{ std::array<Side, 2>{} };
  std::ranges::transform(Sized(record, "sides", sides.size()), sides.begin(), SideOf);
  return sides;
}

auto SetupOf(Json const& record) -> Setup {
  return Setup{ .scenario   = ByteField(record, "scenario"),
                .year       = Number(record, "year", 0xFFFF),
                .month      = ByteField(record, "month"),
                .day        = ByteField(record, "day"),
                .hour       = ByteField(record, "hour"),
                .weather    = ByteField(record, "weather"),
                .difficulty = ByteField(record, "difficulty"),
                .sides      = SidesOf(record),
                .people     = Listed(record, PEOPLE, PersonOf),
                .elements   = Listed(record, ELEMENTS, ElementOf),
                .units      = Listed(record, UNITS, UnitOf),
                .objectives = Listed(record, OBJECTIVES, ObjectiveOf) };
}

auto ConditionsOf(Json const& record) -> std::array<std::string, 2> {
  auto const& conditions{ Sized(record, "conditions", 2) };
  return { conditions.at(0).get<std::string>(), conditions.at(1).get<std::string>() };
}

auto FileOf(Json const& table) -> ScenarioFile {
  auto        descriptions{ ScenarioFile::Descriptions{} };
  auto        conditions{ ScenarioFile::Conditions{} };
  auto        setups{ ScenarioFile::Setups{} };
  auto const& scenarios{ Sized(table, "scenarios", scenario::SCENARIOS) };
  std::ranges::transform(scenarios, descriptions.begin(), [](Json const& record) { return TextOf(record, "description"); });
  std::ranges::transform(scenarios, conditions.begin(), ConditionsOf);
  std::ranges::transform(Sized(table, "setups", scenario::SETUPS), setups.begin(), SetupOf);
  return ScenarioFile{ std::move(descriptions), std::move(conditions), std::move(setups) };
}

}

auto ScenarioTableText(ScenarioFile const& file) -> std::string {
  auto table         = Json::object();
  table["scenarios"] = Each(Indices(scenario::SCENARIOS), [&file](std::size_t const n) { return ScenarioJson(file, n); });
  table["setups"]    = Each(Indices(scenario::SETUPS), [&file](std::size_t const n) { return SetupJson(file.SetupOf(n)); });
  return TableText("scenario_file", std::move(table));
}

auto ReadScenarioTable(Bytes const bytes) -> Packed<ScenarioFile> {
  return ReadTable(bytes, "scenario_file", FileOf);
}

}
