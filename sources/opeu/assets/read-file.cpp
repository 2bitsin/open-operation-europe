#include "opeu/assets/read-file.hpp"

#include <format>
#include <fstream>
#include <iterator>
#include <ranges>

namespace opeu::assets::detail::read_file {

auto ReadFile(std::filesystem::path const& path) -> std::expected<std::vector<std::byte>, std::string> {
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
