#include "opeu/assets/source.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
#include <ranges>

namespace opeu::assets::detail::source {

namespace {

auto ReadAll(std::filesystem::path const& path) -> std::expected<std::vector<std::byte>, std::string> {
  auto file{
    std::ifstream{ path, std::ios::binary }
  };
  if (!file) return std::unexpected{ std::format("assets: cannot read {}", path.string()) };
  auto const bytes{
    std::vector<char>{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} }
  };
  return bytes | std::views::transform([](char const c) { return static_cast<std::byte>(c); }) | std::ranges::to<std::vector>();
}

}

auto DirectorySource::Open(std::filesystem::path const& directory) -> std::expected<DirectorySource, std::string> {
  auto       error{ std::error_code{} };
  auto const entries{
    std::filesystem::directory_iterator{ directory, error }
  };
  if (error) return std::unexpected{ std::format("assets: cannot list {}: {}", directory.string(), error.message()) };
  auto contents{ Contents{} };
  for (auto const& entry : entries) {
    if (!entry.is_regular_file()) continue;
    auto bytes{ ReadAll(entry.path()) };
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
