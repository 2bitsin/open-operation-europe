// The native files: each method forwards to its fileio.cpp function (the DOS handle layer).
#include "native-machine.hpp"

#include "services.hpp"

#include <string>
#include <utility>

namespace opeu::services::detail::native_machine {
namespace {

constexpr std::int16_t FAILED{ -1 };

[[nodiscard]] auto HandleOf(std::int16_t const handle) -> std::optional<FileHandle> {
  if (handle == FAILED) return std::nullopt;
  return FileHandle{ handle };
}

[[nodiscard]] auto CountOf(std::int16_t const count) -> std::optional<std::uint16_t> {
  if (count == FAILED) return std::nullopt;
  return static_cast<std::uint16_t>(count);
}

// The DOS layer takes NUL-terminated names.
[[nodiscard]] auto Terminated(std::string_view const name) -> std::string {
  return std::string{ name };
}

}

auto NativeFiles::Open(std::string_view const name, OpenMode const mode) -> std::optional<FileHandle> {
  return HandleOf(opeu_dos_open(Terminated(name).c_str(), std::to_underlying(mode)));
}

auto NativeFiles::Create(std::string_view const name) -> std::optional<FileHandle> {
  return HandleOf(opeu_dos_create(Terminated(name).c_str()));
}

auto NativeFiles::Exists(std::string_view const name) -> bool {
  return opeu_dos_exists(Terminated(name).c_str()) != 0;
}

auto NativeFiles::Close(FileHandle const file) -> void {
  opeu_dos_close(std::to_underlying(file));
}

auto NativeFiles::Read(FileHandle const file, std::span<std::uint8_t> const into) -> std::optional<std::uint16_t> {
  return CountOf(opeu_dos_read(std::to_underlying(file), into.data(), static_cast<std::uint16_t>(into.size())));
}

auto NativeFiles::ReadFar(FileHandle const file, FarRegion const into) -> std::optional<std::uint16_t> {
  return CountOf(opeu_dos_read_far(std::to_underlying(file), into.at.segment, into.at.offset, into.size));
}

auto NativeFiles::Write(FileHandle const file, std::span<std::uint8_t const> const bytes) -> std::optional<std::uint16_t> {
  return CountOf(opeu_dos_write(std::to_underlying(file), bytes.data(), static_cast<std::uint16_t>(bytes.size())));
}

auto NativeFiles::Seek(FileHandle const file, std::int32_t const position, SeekOrigin const origin) -> std::optional<std::int32_t> {
  auto const reached{ opeu_dos_seek(std::to_underlying(file), position, std::to_underlying(origin)) };
  if (reached == FAILED) return std::nullopt;
  return reached;
}

auto NativeFiles::ResetDisk() -> void {
  port_disk_reset();
}

}
