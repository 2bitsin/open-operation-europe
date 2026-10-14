#include "opeu/assets/messages.hpp"

#include "opeu/assets/table-json.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace opeu::assets::detail::messages {

namespace {

using namespace table_json;
using Entry = std::pair<std::string, std::string>;

auto Descriptions(ScenarioFile const& file) -> std::vector<Entry> {
  return Indices(scenario::SCENARIOS) | std::views::transform([&file](std::size_t const n) { return Entry{ std::format("scenario/{}/description", n), file.Description(n) }; })
         | std::ranges::to<std::vector>();
}

auto Conditions(ScenarioFile const& file) -> std::vector<Entry> {
  return Grid(scenario::SCENARIOS, 2) | std::views::transform([&file](auto const scenario_side) {
           return Entry{ std::format("scenario/{}/condition/{}", scenario_side.first, scenario_side.second), file.Condition(scenario_side.first, scenario_side.second) };
         })
         | std::ranges::to<std::vector>();
}

// The non-empty names of a set-up's records of one kind, `word` naming the kind in the ids.
template <class _Record>
auto Names(std::size_t const setup, std::vector<_Record> const& records, std::string_view const word) -> std::vector<Entry> {
  return Indices(records.size()) | std::views::filter([&records](std::size_t const n) { return !records[n].name.empty(); })
         | std::views::transform([&](std::size_t const n) { return Entry{ std::format("setup/{}/{}/{:02}/name", setup, word, n), records[n].name }; }) | std::ranges::to<std::vector>();
}

auto SetupNames(ScenarioFile const& file) -> std::vector<Entry> {
  return Indices(scenario::SETUPS)
         | std::views::transform([&file](std::size_t const n) { return std::array{ Names(n, file.SetupOf(n).people, "person"), Names(n, file.SetupOf(n).objectives, "objective") }; })
         | std::views::join | std::views::join | std::ranges::to<std::vector>();
}

auto MessagesIn(Json const& table) -> Messages {
  if (!table.is_object()) throw std::invalid_argument{ "messages is not an object of id and text" };
  return table.get<std::map<std::string, std::string>>() | std::ranges::to<Messages>();
}

}

auto ScenarioTexts(ScenarioFile const& file) -> Messages {
  return std::array{ Descriptions(file), Conditions(file), SetupNames(file) } | std::views::join | std::ranges::to<Messages>();
}

auto MessagesOf(Messages const& texts, CodeTable const& table) -> Messages {
  return texts | std::views::transform([&table](auto const& text) { return Entry{ text.first, table.Decode(text.second) }; }) | std::ranges::to<Messages>();
}

auto MessagesText(Messages const& messages) -> std::string {
  auto table = Json::object();
  std::ranges::for_each(messages, [&table](auto const& message) { table[message.first] = message.second; });
  return TableText("messages", std::move(table));
}

auto ReadMessages(Bytes const bytes) -> Packed<Messages> {
  return ReadTable(bytes, "messages", MessagesIn);
}

}
