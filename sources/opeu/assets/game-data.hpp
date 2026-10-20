#pragma once
// The game's data: the 26 files of the release (data-files.hpp), from the table compiled into the executable or from a
// player's directory, checked file by file. The file layer, the content index and the derived pack all read them
// through one GameData, which the application owns for the run.

#include "opeu/assets/data-files.hpp"
#include "opeu/assets/source.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::game_data {

// What a directory holds of one expected file.
enum class FileVerdict : std::uint8_t {
  PRESENT,       // the right size and fingerprint
  MISSING,       // no file of that name, in any case
  UNREADABLE,    // the file exists but cannot be read
  WRONG_SIZE,    // another size: a damaged or truncated copy
  OTHER_EDITION, // the right size but other bytes: another edition or a patched copy
};

[[nodiscard]] auto Describe(FileVerdict verdict) noexcept -> std::string_view;

struct FileCheck {
  DataFile    expected;
  FileVerdict verdict{};
};

// One candidate directory, checked: every expected file's verdict, or why the directory could not be listed.
struct DirectoryCheck {
  std::filesystem::path  directory;
  std::string            unlisted; // empty when the directory was listed
  std::vector<FileCheck> files;

  [[nodiscard]] auto Complete() const -> bool;
  [[nodiscard]] auto Faults() const -> std::size_t; // the files that are not PRESENT, or every file when unlisted
  [[nodiscard]] auto Text() const -> std::string;   // one line per file, with its verdict
};

// Why no candidate holds the game's data: every directory tried, in order, and what the files must be.
struct DataReport {
  std::vector<DirectoryCheck> checked;
  std::span<DataFile const>   expected;

  [[nodiscard]] auto Text() const -> std::string;
};

class GameData final : public Source {
public:
  using Table = std::map<std::string, std::span<std::uint8_t const>, std::less<>>;

  // The files compiled into the executable, by name; nothing is copied, the table outlives the run.
  [[nodiscard]] static auto FromTable(Table files) -> GameData;
  // The expected files of `directory`, matched by name in any case (the upper-case spelling wins) and checked by size
  // and fingerprint; anything else in the directory is ignored. The report when a file is missing or not the release's.
  [[nodiscard]] static auto FromDirectory(std::filesystem::path const& directory, std::span<DataFile const> expected) -> std::expected<GameData, DataReport>;

  GameData(GameData const&)                        = delete;
  GameData(GameData&&) noexcept                    = default;
  auto operator=(GameData const&) -> GameData&     = delete;
  auto operator=(GameData&&) noexcept -> GameData& = default;
  ~GameData() override                             = default;

  [[nodiscard]] auto File(std::string_view name) const -> Decoded<Bytes> override;
  // A DOS file name as the game opens it ("c:\\opeu\\msg.16p"): the drive and directories dropped, the name upper-cased.
  [[nodiscard]] auto DosFile(std::string_view dos_name) const -> std::optional<std::span<std::uint8_t const>>;
  [[nodiscard]] auto Names() const -> std::vector<std::string>; // sorted

private:
  using Owned = std::vector<std::vector<std::uint8_t>>;
  GameData(Table files, Owned owned);

  Owned _owned; // a directory's bytes; the spans of _files point into them (a move keeps every buffer where it is)
  Table _files;
};

// What `directory` holds of the expected files.
[[nodiscard]] auto CheckDirectory(std::filesystem::path const& directory, std::span<DataFile const> expected) -> DirectoryCheck;

// The first candidate directory that holds every expected file, or the report of all of them.
[[nodiscard]] auto FindGameData(std::span<std::filesystem::path const> candidates, std::span<DataFile const> expected) -> std::expected<GameData, DataReport>;

}

namespace opeu::assets {
using detail::game_data::CheckDirectory;
using detail::game_data::Describe;
using detail::game_data::DataReport;
using detail::game_data::DirectoryCheck;
using detail::game_data::FileCheck;
using detail::game_data::FileVerdict;
using detail::game_data::FindGameData;
using detail::game_data::GameData;
}
