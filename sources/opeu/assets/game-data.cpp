#include "opeu/assets/game-data.hpp"

#include "opeu/assets/fingerprint.hpp"
#include "opeu/assets/read-file.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <limits>
#include <map>
#include <ranges>
#include <system_error>
#include <utility>

namespace opeu::assets::detail::game_data {
namespace {

// DOS names are ASCII.
auto UpperCase(std::string_view const text) -> std::string {
  return text | std::views::transform([](char const c) { return 'a' <= c && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c; }) | std::ranges::to<std::string>();
}

// A directory's regular files by upper-case name; where two spellings of one name exist, the upper-case one.
using Listing = std::map<std::string, std::filesystem::path, std::less<>>;

auto List(std::filesystem::path const& directory) -> std::expected<Listing, std::string> {
  auto       error{ std::error_code{} };
  auto const entries{
    std::filesystem::directory_iterator{ directory, error }
  };
  if (error) return std::unexpected{ error.message() };
  auto listing{ Listing{} };
  auto files{ entries | std::views::filter([](std::filesystem::directory_entry const& entry) { return entry.is_regular_file(); }) };
  std::ranges::for_each(files, [&listing](std::filesystem::directory_entry const& entry) {
    auto const name{ entry.path().filename().string() };
    auto const upper{ UpperCase(name) };
    if (name == upper || !listing.contains(upper)) listing.insert_or_assign(upper, entry.path());
  });
  return listing;
}

// One expected file as a directory holds it: the verdict, and the bytes when it is the release's.
struct Found {
  FileVerdict               verdict{};
  std::vector<std::uint8_t> bytes;
};

auto Verdict(DataFile const& expected, Bytes const bytes) -> FileVerdict {
  if (bytes.size() != expected.size) return FileVerdict::WRONG_SIZE;
  if (FingerprintOf(bytes) != expected.fingerprint) return FileVerdict::OTHER_EDITION;
  return FileVerdict::PRESENT;
}

auto FindFile(Listing const& listing, DataFile const& expected) -> Found {
  auto const listed{ listing.find(expected.name) };
  if (listed == listing.end()) return Found{ .verdict = FileVerdict::MISSING, .bytes = {} };
  auto bytes{ ReadFile(listed->second) };
  if (!bytes) return Found{ .verdict = FileVerdict::UNREADABLE, .bytes = {} };
  auto const verdict{ Verdict(expected, *bytes) };
  if (verdict != FileVerdict::PRESENT) return Found{ .verdict = verdict, .bytes = {} };
  return Found{ .verdict = verdict, .bytes = *bytes | std::views::transform(std::to_integer<std::uint8_t>) | std::ranges::to<std::vector>() };
}

// A directory checked, with the bytes of every file that is the release's, in the order of the expected files.
struct Scan {
  DirectoryCheck                         check;
  std::vector<std::vector<std::uint8_t>> bytes;
};

auto ScanDirectory(std::filesystem::path const& directory, std::span<DataFile const> const expected) -> Scan {
  auto scan{
    Scan{ .check = DirectoryCheck{ .directory = directory, .unlisted = {}, .files = {} }, .bytes = {} }
  };
  auto const listing{ List(directory) };
  if (!listing) {
    scan.check.unlisted = listing.error();
    return scan;
  }
  std::ranges::for_each(expected, [&](DataFile const& file) {
    auto found{ FindFile(*listing, file) };
    scan.check.files.push_back(FileCheck{ .expected = file, .verdict = found.verdict });
    scan.bytes.push_back(std::move(found.bytes));
  });
  return scan;
}

auto FileLine(FileCheck const& file) -> std::string {
  return std::format("  {:<12} {:>7} bytes  {}\n", file.expected.name, file.expected.size, Describe(file.verdict));
}

}

auto Describe(FileVerdict const verdict) noexcept -> std::string_view {
  switch (verdict) {
  case FileVerdict::PRESENT:       return "ok";
  case FileVerdict::MISSING:       return "missing";
  case FileVerdict::UNREADABLE:    return "cannot be read";
  case FileVerdict::WRONG_SIZE:    return "wrong size (a damaged or incomplete copy)";
  case FileVerdict::OTHER_EDITION: return "other bytes (another edition or a modified copy)";
  }
  return "unknown";
}

auto DirectoryCheck::Complete() const -> bool {
  return unlisted.empty() && std::ranges::all_of(files, [](FileCheck const& file) { return file.verdict == FileVerdict::PRESENT; });
}

auto DirectoryCheck::Faults() const -> std::size_t {
  if (!unlisted.empty()) return std::numeric_limits<std::size_t>::max();
  return static_cast<std::size_t>(std::ranges::count_if(files, [](FileCheck const& file) { return file.verdict != FileVerdict::PRESENT; }));
}

auto DirectoryCheck::Text() const -> std::string {
  if (!unlisted.empty()) return std::format("{}: cannot be listed: {}\n", directory.string(), unlisted);
  return std::format("{}:\n", directory.string()) + (files | std::views::transform(FileLine) | std::views::join | std::ranges::to<std::string>());
}

auto DataReport::Text() const -> std::string {
  auto text{ std::format("The game needs the {} data files of {}.\n", expected.size(), RELEASE) };
  if (checked.empty()) return text;
  text += "Looked in:\n";
  std::ranges::for_each(checked, [&text](DirectoryCheck const& check) {
    text += check.unlisted.empty() ? std::format("  {}: {} of the files missing or not the release's\n", check.directory.string(), check.Faults())
                                   : std::format("  {}: cannot be listed: {}\n", check.directory.string(), check.unlisted);
  });
  auto const closest{ std::ranges::min_element(checked, std::ranges::less{}, &DirectoryCheck::Faults) };
  if (closest->unlisted.empty()) text += "The closest, file by file: " + closest->Text();
  return text;
}

GameData::GameData(Table files, Owned owned) : _owned{ std::move(owned) }, _files{ std::move(files) } { }

auto GameData::FromTable(Table files) -> GameData {
  return GameData{ std::move(files), Owned{} };
}

auto GameData::FromDirectory(std::filesystem::path const& directory, std::span<DataFile const> const expected) -> std::expected<GameData, DataReport> {
  auto scan{ ScanDirectory(directory, expected) };
  if (!scan.check.Complete())
    return std::unexpected{
      DataReport{ .checked = { std::move(scan.check) }, .expected = expected }
    };
  auto files{ Table{} };
  std::ranges::for_each(std::views::zip(expected, scan.bytes), [&files](auto const& file) { files.emplace(std::get<0>(file).name, std::span<std::uint8_t const>{ std::get<1>(file) }); });
  return GameData{ std::move(files), std::move(scan.bytes) };
}

auto GameData::File(std::string_view const name) const -> Decoded<Bytes> {
  if (auto const found{ _files.find(name) }; found != _files.end()) return std::as_bytes(found->second);
  return std::unexpected{ DecodeError::MISSING_FILE };
}

auto GameData::DosFile(std::string_view dos_name) const -> std::optional<std::span<std::uint8_t const>> {
  if (auto const last{ dos_name.find_last_of("\\/:") }; last != std::string_view::npos) dos_name.remove_prefix(last + 1);
  auto const found{ _files.find(UpperCase(dos_name)) };
  if (found == _files.end()) return std::nullopt;
  return found->second;
}

auto GameData::Names() const -> std::vector<std::string> {
  return _files | std::views::keys | std::ranges::to<std::vector>();
}

auto CheckDirectory(std::filesystem::path const& directory, std::span<DataFile const> const expected) -> DirectoryCheck {
  return ScanDirectory(directory, expected).check;
}

auto FindGameData(std::span<std::filesystem::path const> const candidates, std::span<DataFile const> const expected) -> std::expected<GameData, DataReport> {
  auto report{
    DataReport{ .checked = {}, .expected = expected }
  };
  // A search: the first complete candidate ends it, and a directory is read only when the ones before it failed.
  for (auto const& candidate : candidates) {
    auto found{ GameData::FromDirectory(candidate, expected) };
    if (found) return found;
    std::ranges::move(found.error().checked, std::back_inserter(report.checked));
  }
  return std::unexpected{ std::move(report) };
}

}
