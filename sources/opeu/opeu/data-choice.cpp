#include "data-choice.hpp"

#include "opeu/assets/data-files.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <ranges>
#include <system_error>
#include <utility>

namespace opeu::app::detail::data_choice {
namespace {

// The current directory, if the process can tell it.
auto CurrentDirectory() -> std::optional<std::filesystem::path> {
  auto failure{ std::error_code{} };
  auto directory{ std::filesystem::current_path(failure) };
  if (failure) return std::nullopt;
  return directory;
}

// The verdict on one directory: each file's, and whether all are the release's.
auto Checked(assets::DirectoryCheck const& check) -> DataCheck {
  return DataCheck{ .text = check.Text(), .complete = check.Complete() };
}

}

auto SearchPlaces(std::optional<std::string_view> const environment, std::vector<std::filesystem::path> host) -> std::vector<std::filesystem::path> {
  auto places{ std::vector<std::filesystem::path>{} };
  if (environment && !environment->empty()) places.emplace_back(*environment);
  std::ranges::move(host, std::back_inserter(places));
  if (auto current{ CurrentDirectory() }) places.push_back(*std::move(current));
  return places;
}

auto ChooseData(DataPlaces const& places, std::optional<assets::GameData::Table> embedded) -> std::expected<assets::GameData, assets::DataReport> {
  if (places.named) return assets::GameData::FromDirectory(*places.named, assets::DATA_FILES);
  if (embedded) return assets::GameData::FromTable(*std::move(embedded));
  return assets::FindGameData(places.search, assets::DATA_FILES);
}

auto CheckData(DataPlaces const& places, std::optional<assets::GameData::Table> const& embedded) -> DataCheck {
  if (places.named) return Checked(assets::CheckDirectory(*places.named, assets::DATA_FILES));
  if (embedded) return DataCheck{ .text = std::format("embedded: the {} data files of {} are compiled into this build\n", assets::DATA_FILES.size(), assets::RELEASE), .complete = true };
  auto const checks{ places.search | std::views::transform([](std::filesystem::path const& place) { return assets::CheckDirectory(place, assets::DATA_FILES); }) | std::ranges::to<std::vector>() };
  auto const found{ std::ranges::find_if(checks, &assets::DirectoryCheck::Complete) };
  if (found != checks.end()) return Checked(*found);
  return DataCheck{
    .text = assets::DataReport{ .checked = checks, .expected = assets::DATA_FILES }
      .Text(), .complete = false
  };
}

}
