// FILEIO: the KOEI DOS file and memory wrappers, natively over the platform's save files.
//
// DOS handles: 0..4 are the predefined devices; files get the lowest free handle from 5 up to 19 (the 20-entry job file
// table of DOS). Results follow the game's library exactly:
//  - open/create: the handle, or -1 when DOS sets CF;
//  - close: always 0 (CF is discarded);
//  - read/write (near and far): raw AX, i.e. the byte count, or the DOS error code when DOS failed (6 = invalid
//    handle; CF is ignored);
//  - seek: DX:AX, the new position; on error AX = the DOS error code and DX keeps the value it was loaded with (the
//    position's low word);
//  - exists: open(name, 0) and close; 1 when both succeed, else 0;
//  - alloc: the segment or 0; free: 1 on success, 0 on failure.
//
// Open mode (AL of 3Dh): 0 read, 1 write, 2 read/write.
//
// Where files are: the shipped data files are compiled into the executable (asset-table.hpp); only the save directory
// (platform.hpp) is on disk. A name is looked up in the save directory first, so a file there shadows the shipped one,
// then in the table. A shipped file is served from memory. The game opens every data file read/write (mode 2) though
// it writes only SAVEDATA; should it ever write to a shipped file, the file is first copied into the save directory
// (copy on write) and the handle continues on that copy. A read/write open of a save-directory file that cannot be
// written falls back to read-only.
#include "asset-table.hpp"
#include "file-table.hpp"
#include "platform.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace opeu::services::detail::file_table {
namespace {

// The one table of the process. The game-facing file functions reach it until services::Host owns it.
auto TheFileTable() -> FileTable& {
  static FileTable files;
  return files;
}

// Runs `action` on the file's save-directory stream, if it is one.
auto WhenSaved(OpenFile& file, auto const action) -> void {
  std::visit(
      [&action](auto& source) {
        if constexpr (std::same_as<std::remove_cvref_t<decltype(source)>, SaveFile>) action(source);
      },
      file.source);
}

auto OpenSaved(std::string_view const name, Access const access) -> std::optional<SaveFile> {
  if (access == Access::READ) return SaveFile::Open(name, FileMode::READ);
  if (auto writable{ SaveFile::Open(name, FileMode::READ_WRITE) }) return writable;
  auto readable{ SaveFile::Open(name, FileMode::READ) };
  if (readable) port_trace("svc", "dos_open %s: read/write refused, opened read-only", std::string{ name }.c_str());
  return readable;
}

}

auto FileTable::Taken(std::int16_t const handle) -> std::optional<OpenFile>& {
  return _handles.at(static_cast<std::size_t>(handle));
}

auto FileTable::Valid(std::int16_t const handle) -> bool {
  return std::cmp_greater_equal(handle, DOS_FIRST_FILE) && std::cmp_less(handle, DOS_HANDLES) && Taken(handle).has_value();
}

auto FileTable::Lookup(std::string_view const name) const -> std::optional<std::span<std::uint8_t const>> {
  return _find ? _find(name) : std::nullopt;
}

// The lowest free handle, or -1 (DOS error 4: too many open files; the file is closed again).
auto FileTable::NewHandle(OpenFile file) -> std::int16_t {
  auto const handles{ std::span{ _handles }.subspan(DOS_FIRST_FILE) };
  auto const free{ std::ranges::find_if(handles, [](std::optional<OpenFile> const& file) { return !file; }) };
  if (free == handles.end()) return -1;
  free->emplace(std::move(file));
  return static_cast<std::int16_t>(DOS_FIRST_FILE + static_cast<std::size_t>(free - handles.begin()));
}

auto FileTable::Open(std::string_view const name, std::uint16_t const mode) -> std::int16_t {
  auto const code{ mode & 7 };
  if (code > 2) return -1;
  auto const access{ static_cast<Access>(code) };
  auto const remembered{ std::string{ name.substr(0, NAME_LENGTH) } };
  if (auto saved{ OpenSaved(name, access) }) return NewHandle(OpenFile{ .source = std::move(*saved), .name = remembered, .access = access });
  if (auto const shipped{ Lookup(name) }) return NewHandle(OpenFile{ .source = ShippedFile{ *shipped }, .name = remembered, .access = access });
  return -1;
}

auto FileTable::Create(std::string_view const name) -> std::int16_t {
  auto created{ SaveFile::Open(name, FileMode::CREATE) };
  if (!created) return -1;
  return NewHandle(OpenFile{ .source = std::move(*created), .name = std::string{ name.substr(0, NAME_LENGTH) }, .access = Access::READ_WRITE });
}

auto FileTable::Close(std::int16_t const handle) -> void {
  if (Valid(handle)) Taken(handle).reset();
}

// A stream needs a positioning call between reads and writes.
auto FileTable::SwitchDirection(OpenFile& file, bool const writing) -> void {
  if (file.last_write == writing) return;
  WhenSaved(file, [](SaveFile& saved) { saved.Seek(0, SeekOrigin::CURRENT); });
  file.last_write = writing;
}

// The first write to a shipped file: its bytes become a file of the save directory, and the handle carries on there
// at the same position.
auto FileTable::CopyOnWrite(OpenFile& file) -> bool {
  auto const& shipped{ std::get<ShippedFile>(file.source) };
  auto        copy{ SaveFile::Open(file.name, FileMode::CREATE) };
  if (!copy) return false;
  auto const bytes{ shipped.Bytes() };
  if ((!bytes.empty() && copy->Write(bytes) != bytes.size()) || !copy->Seek(shipped.Position(), SeekOrigin::START)) return false;
  port_trace("svc", "dos_write %s: copied the shipped file into the save directory", file.name.c_str());
  file.source = std::move(*copy);
  return true;
}

auto FileTable::Read(std::int16_t const handle, std::span<std::uint8_t> const bytes) -> std::int16_t {
  if (!Valid(handle)) return INVALID_HANDLE;
  auto& file{ *Taken(handle) };
  if (file.access == Access::WRITE) return ACCESS_DENIED;
  SwitchDirection(file, false);
  auto const count{ std::visit([bytes](auto& source) { return source.Read(bytes); }, file.source) };
  return static_cast<std::int16_t>(static_cast<std::uint16_t>(count));
}

auto FileTable::Write(std::int16_t const handle, std::span<std::uint8_t const> const bytes) -> std::int16_t {
  if (!Valid(handle)) return INVALID_HANDLE;
  auto& file{ *Taken(handle) };
  if (file.access == Access::READ) return ACCESS_DENIED;
  SwitchDirection(file, true);
  if (bytes.empty()) return 0; // DOS truncates at the position; nothing the game relies on
  if (std::holds_alternative<ShippedFile>(file.source) && !CopyOnWrite(file)) return ACCESS_DENIED;
  return static_cast<std::int16_t>(static_cast<std::uint16_t>(std::get<SaveFile>(file.source).Write(bytes)));
}

// DOS takes CX:DX as a signed 32-bit offset for origins 1 and 2 and as unsigned for 0.
auto FileTable::Seek(std::int16_t const handle, std::int32_t const position, std::int16_t const origin) -> std::int32_t {
  auto const low{ static_cast<std::uint32_t>(static_cast<std::uint16_t>(position)) << 16 };
  auto const code{ static_cast<std::uint8_t>(origin) };
  if (!Valid(handle)) return static_cast<std::int32_t>(low | INVALID_HANDLE);
  if (code > 2) return static_cast<std::int32_t>(low | INVALID_FUNCTION);
  auto&      file{ *Taken(handle) };
  auto const from{ static_cast<SeekOrigin>(code) };
  auto const offset{ from == SeekOrigin::START ? std::int64_t{ static_cast<std::uint32_t>(position) } : std::int64_t{ position } };
  auto const reached{ std::visit([offset, from](auto& source) { return source.Seek(offset, from); }, file.source) };
  file.last_write = false;
  port_trace("svc", "dos_seek(%d, %ld, %d) = %ld", handle, static_cast<long>(position), origin, static_cast<long>(reached.value_or(-1)));
  if (!reached) return static_cast<std::int32_t>(low | SEEK_ERROR);
  return static_cast<std::int32_t>(static_cast<std::uint32_t>(*reached));
}

auto FileTable::FlushAll() -> void {
  std::ranges::for_each(_handles, [](std::optional<OpenFile>& file) {
    if (file) WhenSaved(*file, [](SaveFile& saved) { saved.Flush(); });
  });
}

namespace {

// The far buffer of size bytes at segment:offset (fatal when it leaves the arena).
auto FarBytes(std::uint16_t const segment, std::uint16_t const offset, std::uint16_t const size) -> std::span<std::uint8_t> {
  if (size != 0) port_far(segment, static_cast<std::uint16_t>(offset + size - 1)); // range check
  return { port_far(segment, offset), size };
}

}
}

namespace opeu::services::detail::asset_table {
auto FilesUseAssets(AssetLookup lookup) -> void {
  file_table::TheFileTable().UseAssets(std::move(lookup));
}
}

// ---- the game's file calls (services.hpp; renamed with #17) ----

using opeu::services::detail::file_table::FarBytes;
using opeu::services::detail::file_table::TheFileTable;

auto opeu_dos_open(char const* const name, std::uint16_t const mode) -> std::int16_t {
  auto const handle{ TheFileTable().Open(name, mode) };
  port_trace("svc", "dos_open(%s, %u) = %d", name, mode, handle);
  return handle;
}

auto opeu_dos_close(std::int16_t const handle) -> std::int16_t {
  TheFileTable().Close(handle);
  port_trace("svc", "dos_close(%d)", handle);
  return 0;
}

auto opeu_dos_read(std::int16_t const handle, std::uint8_t* const data, std::uint16_t const size) -> std::int16_t {
  auto const result{ TheFileTable().Read(handle, { data, size }) };
  port_trace("svc", "dos_read(%d, %u) = %d", handle, size, result);
  return result;
}

auto opeu_dos_read_far(std::int16_t const handle, std::uint16_t const segment, std::uint16_t const offset, std::uint16_t const size) -> std::int16_t {
  auto const result{ TheFileTable().Read(handle, FarBytes(segment, offset, size)) };
  port_trace("svc", "dos_read_far(%d, %04x:%04x, %u) = %d", handle, segment, offset, size, result);
  return result;
}

auto opeu_dos_write(std::int16_t const handle, std::uint8_t const* const data, std::uint16_t const size) -> std::int16_t {
  auto const result{ TheFileTable().Write(handle, { data, size }) };
  port_trace("svc", "dos_write(%d, %u) = %d", handle, size, result);
  return result;
}

auto opeu_dos_write_far(std::int16_t const handle, std::uint16_t const segment, std::uint16_t const offset, std::uint16_t const size) -> std::int16_t {
  auto const result{ TheFileTable().Write(handle, FarBytes(segment, offset, size)) };
  port_trace("svc", "dos_write_far(%d, %04x:%04x, %u) = %d", handle, segment, offset, size, result);
  return result;
}

auto opeu_dos_seek(std::int16_t const handle, std::int32_t const position, std::int16_t const origin) -> std::int32_t {
  return TheFileTable().Seek(handle, position, origin);
}

auto opeu_dos_create(char const* const name) -> std::int16_t {
  auto const handle{ TheFileTable().Create(name) };
  port_trace("svc", "dos_create(%s) = %d", name, handle);
  return handle;
}

auto opeu_dos_exists(char const* const name) -> std::int16_t {
  auto const handle{ opeu_dos_open(name, 0) };
  if (handle == -1) return 0;
  if (opeu_dos_close(handle) == -1) return 0;
  return 1;
}

auto opeu_dos_alloc(std::uint16_t const paragraphs) -> std::uint16_t {
  return port_dos_alloc(paragraphs);
}

auto opeu_dos_free(std::uint16_t const segment) -> std::int16_t {
  return port_dos_free(segment) == 0 ? 1 : 0;
}

// INT 21h AH=0Dh: DOS writes its dirty buffers; flush every open file.
auto port_disk_reset() -> void {
  TheFileTable().FlushAll();
}
