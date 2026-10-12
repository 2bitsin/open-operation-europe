#include "opeu/assets/pack-files.hpp"

#include "opeu/assets/read-file.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <ranges>

namespace opeu::assets::detail::pack_files {

namespace {

using namespace std::string_view_literals;

constexpr auto MAGIC{ "OPEUPACK"sv };
constexpr auto LENGTH_BYTES{ std::size_t{ 4 } };
constexpr auto ARCHIVE_FORMAT{ "opeu-archive/1"sv };

// A relative path that stays inside the pack: no root, no "..".
auto Contained(std::filesystem::path const& path) -> bool {
  return path.is_relative() && !path.empty() && std::ranges::none_of(path, [](std::filesystem::path const& part) { return part == ".."; });
}

auto HasMagic(Bytes const archive) -> bool {
  return archive.size() >= MAGIC.size() + LENGTH_BYTES && std::ranges::equal(archive.first(MAGIC.size()), MAGIC, [](std::byte const b, char const c) { return std::to_integer<char>(b) == c; });
}

auto DirectoryLength(Bytes const archive) -> std::size_t {
  return ReadWord(archive, MAGIC.size()) | std::size_t{ ReadWord(archive, MAGIC.size() + 2) } << 16;
}

}

auto DirectoryPackFiles::Read(std::string_view const path) const -> Packed<std::vector<std::byte>> {
  auto const relative{ std::filesystem::path{ path } };
  if (!Contained(relative)) return Fail(PackFault::BAD_MANIFEST, std::format("{} leaves the pack", path));
  auto bytes{ ReadFile(_root / relative) };
  if (!bytes) return Fail(PackFault::MISSING_FILE, bytes.error());
  return *std::move(bytes);
}

auto ArchivePackFiles::Open(std::filesystem::path const& path) -> Packed<ArchivePackFiles> {
  auto bytes{ ReadFile(path) };
  if (!bytes) return Fail(PackFault::MISSING_FILE, bytes.error());
  return FromBytes(*std::move(bytes));
}

auto ArchivePackFiles::FromBytes(std::vector<std::byte> archive) -> Packed<ArchivePackFiles> {
  if (!HasMagic(archive)) return Fail(PackFault::BAD_ARCHIVE, "no OPEUPACK header");
  auto const length{ DirectoryLength(archive) };
  auto const directory{ Slice(archive, MAGIC.size() + LENGTH_BYTES, length) };
  if (!directory) return Fail(PackFault::BAD_ARCHIVE, "the directory runs past the end");
  auto const body{ MAGIC.size() + LENGTH_BYTES + length };
  try {
    auto const index = nlohmann::json::parse(AsText(*directory)); // not braces: a braced json is an array holding it
    if (index.at("format").get<std::string>() != ARCHIVE_FORMAT) return Fail(PackFault::BAD_ARCHIVE, "not an opeu-archive/1 directory");
    auto extents{ index.at("files") | std::views::transform([](nlohmann::json const& file) {
                    return std::pair{
                      file.at("path").get<std::string>(), Extent{ .at = file.at("at").get<std::size_t>(), .size = file.at("bytes").get<std::size_t>() }
                    };
                  })
                  | std::ranges::to<Extents>() };
    auto const room{ archive.size() - body };
    if (!std::ranges::all_of(extents | std::views::values, [room](Extent const& e) { return e.at <= room && e.size <= room - e.at; })) {
      return Fail(PackFault::BAD_ARCHIVE, "a file runs past the end");
    }
    return ArchivePackFiles{ std::move(archive), body, std::move(extents) };
  } catch (nlohmann::json::exception const& error) {
    return Fail(PackFault::BAD_ARCHIVE, error.what());
  }
}

auto ArchivePackFiles::Read(std::string_view const path) const -> Packed<std::vector<std::byte>> {
  auto const found{ _extents.find(path) };
  if (found == _extents.end()) return Fail(PackFault::MISSING_FILE, std::string{ path });
  auto const file{ Bytes{ _archive }.subspan(_body + found->second.at, found->second.size) };
  return std::vector<std::byte>{ file.begin(), file.end() };
}

auto ArchivePackFiles::Paths() const -> std::vector<std::string> {
  return _extents | std::views::keys | std::ranges::to<std::vector>();
}

auto MemoryPackFiles::Read(std::string_view const path) const -> Packed<std::vector<std::byte>> {
  if (auto const found{ _files.find(path) }; found != _files.end()) return found->second;
  return Fail(PackFault::MISSING_FILE, std::string{ path });
}

}
