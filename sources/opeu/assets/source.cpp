#include "opeu/assets/source.hpp"

#include "opeu/assets/read-file.hpp"

#include <algorithm>
#include <format>
#include <ranges>

namespace opeu::assets::detail::source {

auto DirectorySource::Open(std::filesystem::path const& directory) -> std::expected<DirectorySource, std::string> {
  auto       error{ std::error_code{} };
  auto const entries{
    std::filesystem::directory_iterator{ directory, error }
  };
  if (error) return std::unexpected{ std::format("assets: cannot list {}: {}", directory.string(), error.message()) };
  auto contents{ Contents{} };
  for (auto const& entry : entries) {
    if (!entry.is_regular_file()) continue;
    auto bytes{ ReadFile(entry.path()) };
    if (!bytes) return std::unexpected{ bytes.error() };
    contents.emplace(entry.path().filename().string(), *std::move(bytes));
  }
  return DirectorySource{ std::move(contents) };
}

auto DirectorySource::File(std::string_view const name) const -> Decoded<Bytes> {
  if (auto const found{ _contents.find(name) }; found != _contents.end()) return Bytes{ found->second };
  return std::unexpected{ DecodeError::MISSING_FILE };
}

auto DirectorySource::Names() const -> std::vector<std::string> {
  return _contents | std::views::keys | std::ranges::to<std::vector>();
}

}
