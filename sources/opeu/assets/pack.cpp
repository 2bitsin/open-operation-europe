#include "opeu/assets/pack.hpp"

#include "opeu/assets/png-reader.hpp"
#include "opeu/contract.hpp"

#include <format>
#include <utility>

namespace opeu::assets::detail::pack {

namespace {

auto WrongSize(PackedAsset const& entry, std::string_view const file) -> std::unexpected<PackError> {
  return Fail(PackFault::BAD_IMAGE, std::format("{}: {} is not the size its entry states", entry.id, file));
}

}

Pack::Pack(std::unique_ptr<PackFiles const> files, Manifest manifest) : _files{ std::move(files) }, _manifest{ std::move(manifest) } {
  Expects(_files != nullptr, "a pack reads its files from somewhere");
}

auto Pack::Open(std::unique_ptr<PackFiles const> files) -> Packed<Pack> {
  auto manifest{ files->Read(MANIFEST_PATH).and_then([](std::vector<std::byte> const& json) { return Manifest::Parse(json); }) };
  if (!manifest) return std::unexpected{ manifest.error() };
  return Pack{ std::move(files), *std::move(manifest) };
}

auto Pack::Find(std::string_view const id) const -> std::optional<PackedAsset> {
  return _manifest.Find(id);
}

auto Pack::Load(std::string_view const id) const -> Packed<LoadedAsset> {
  auto entry{ _manifest.Find(id) };
  if (!entry) return Fail(PackFault::UNKNOWN_ASSET, std::string{ id });
  auto texture{ LoadTexture(*entry) };
  if (!texture) return std::unexpected{ texture.error() };
  auto indexed{ LoadIndexed(*entry, *texture) };
  if (!indexed) return std::unexpected{ indexed.error() };
  return LoadedAsset{ .entry = *std::move(entry), .texture = *std::move(texture), .indexed = *std::move(indexed) };
}

auto Pack::Palette(std::string_view const name) const -> std::optional<PaletteColours> {
  return _manifest.Palette(name);
}

auto Pack::Ids() const -> std::vector<std::string> {
  return _manifest.Ids();
}

auto Pack::Frames(std::string_view const animation) const -> std::optional<std::vector<std::string>> {
  return _manifest.Frames(animation);
}

auto Pack::FontNamed(std::string_view const name) const -> std::optional<Font> {
  return _manifest.FontNamed(name);
}

auto Pack::FontNames() const -> std::vector<std::string> {
  return _manifest.FontNames();
}

auto Pack::Scale() const -> std::uint32_t {
  return _manifest.Scale();
}

auto Pack::LoadTexture(PackedAsset const& entry) const -> Packed<Texture> {
  auto texture{ _files->Read(entry.image).and_then([](std::vector<std::byte> const& png) { return ReadTexture(png); }) };
  if (texture && (texture->Width() != entry.width * _manifest.Scale() || texture->Height() != entry.height * _manifest.Scale())) return WrongSize(entry, entry.image);
  return texture;
}

auto Pack::LoadIndexed(PackedAsset const& entry, Texture const& texture) const -> Packed<std::optional<Image>> {
  if (!entry.indexed) return std::optional<Image>{};
  auto indexed{ _files->Read(*entry.indexed).and_then([](std::vector<std::byte> const& png) { return ReadIndexed(png); }) };
  if (!indexed) return std::unexpected{ indexed.error() };
  if (indexed->Width() != texture.Width() || indexed->Height() != texture.Height()) return WrongSize(entry, *entry.indexed);
  return std::optional{ *std::move(indexed) };
}

auto OpenPack(std::filesystem::path const& path) -> Packed<Pack> {
  if (std::filesystem::is_directory(path)) return Pack::Open(std::make_unique<DirectoryPackFiles const>(path));
  auto archive{ ArchivePackFiles::Open(path) };
  if (!archive) return std::unexpected{ archive.error() };
  return Pack::Open(std::make_unique<ArchivePackFiles const>(*std::move(archive)));
}

}
