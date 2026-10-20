#include "opeu/assets/source.hpp"

#include "opeu/assets/read-file.hpp"

#include <algorithm>
#include <format>
#include <ranges>

namespace opeu::assets::detail::source {

using namespace std::string_literals;
using namespace std::string_view_literals;

auto DirectorySource::Open(std::filesystem::path const& directory) -> std::expected<DirectorySource, std::string> {
  using SDI = std::filesystem::directory_iterator;
  auto       errcode{ std::error_code{} };
  auto const entries{
    SDI{ directory, errcode }
  };
  if (errcode) return std::unexpected{ std::format("assets: cannot list {}: {}", directory.string(), errcode.message()) };
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
