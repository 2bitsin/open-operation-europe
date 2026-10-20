#include "embedded.hpp"

#include "opeu/assets/data-files.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "opeu-embedded/resources.hpp" // written by configure.py: buildutil's accessor, or an empty one

namespace opeu::app::detail::embedded {
namespace {

using embedded_resources::DATA_EMBEDDED;
using embedded_resources::PACK_EMBEDDED;

// The bytes of a file of the accessor, by its path under the *.embed/ directory.
auto Embedded(std::string_view const name) -> std::optional<std::span<std::uint8_t const>> {
  auto const files{ resources::all() };
  auto const found{ std::ranges::find(files, name, &resources::Resource::name) };
  if (found == files.end()) return std::nullopt;
  return std::span<std::uint8_t const>{ found->data, found->size };
}

// The files of one embedded pack: `directory` is the pack's directory under packs.embed/. Indexed once, when made.
class ResourcePackFiles final : public assets::PackFiles {
public:
  explicit ResourcePackFiles(std::string_view const directory) : _directory{ std::format("{}/", directory) } {
    auto in_pack{ resources::all() | std::views::filter([this](resources::Resource const& file) { return file.name.starts_with(_directory); }) };
    std::ranges::for_each(in_pack, [this](resources::Resource const& file) { _files.emplace(file.name.substr(_directory.size()), std::span<std::uint8_t const>{ file.data, file.size }); });
  }

  [[nodiscard]] auto Read(std::string_view const path) const -> assets::Packed<std::vector<std::byte>> override {
    auto const found{ _files.find(path) };
    if (found == _files.end()) return assets::Fail(assets::PackFault::MISSING_FILE, _directory + std::string{ path });
    return found->second | std::views::transform([](std::uint8_t const byte) { return std::byte{ byte }; }) | std::ranges::to<std::vector>();
  }

private:
  std::string                                                            _directory;
  std::map<std::string_view, std::span<std::uint8_t const>, std::less<>> _files; // the accessor's names live as long as the program
};

auto EmbeddedTable() -> assets::GameData::Table {
  auto table{ assets::GameData::Table{} };
  std::ranges::for_each(assets::DATA_FILES, [&table](assets::DataFile const& file) {
    auto const found{ Embedded(file.name) };
    if (found) table.emplace(file.name, *found);
  });
  return table;
}

}

auto EmbeddedData() -> std::optional<assets::GameData::Table> {
  if (!DATA_EMBEDDED) return std::nullopt; // a plain if: the table's code is compiled, and checked, in every build
  return EmbeddedTable();
}

auto EmbeddedPackFiles(std::string_view const directory) -> std::unique_ptr<assets::PackFiles const> {
  if (!PACK_EMBEDDED) return std::make_unique<assets::MemoryPackFiles const>(assets::MemoryPackFiles::Files{});
  return std::make_unique<ResourcePackFiles const>(directory);
}

auto OpenEmbeddedPack(std::string_view const directory) -> std::optional<assets::Packed<assets::Pack>> {
  if (!PACK_EMBEDDED) return std::nullopt;
  return assets::Pack::Open(EmbeddedPackFiles(directory));
}

}
