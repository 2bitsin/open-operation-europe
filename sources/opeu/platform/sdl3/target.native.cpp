// The native targets' side of target.hpp: vsync, SDL_Delay, the preference path for saves and the game's data, and a
// message box when the data is missing.
#include "opeu/platform/sdl3/target.hpp"

#include "opeu/platform/sdl3/handles.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace opeu::platform::detail::target {

auto ExtraWindowFlags() -> SDL_WindowFlags {
  return 0;
}

auto OwnsWindowSize() -> bool {
  return true;
}

auto PrepareHints() -> void { }

auto PaceFrames(SDL_Renderer& renderer) -> void {
  SDL_SetRenderVSync(&renderer, 1);
}

auto AfterFrame() -> void { }

auto Sleep(int const ms, [[maybe_unused]] bool const headless) -> void {
  if (ms > 0) SDL_Delay(static_cast<Uint32>(ms));
}

namespace {
// The per-user directory SDL gives the game (created if need be); nothing when it has none.
auto PreferencePath() -> std::optional<std::filesystem::path> {
  auto const pref{ std::unique_ptr<char, Destroy<SDL_free>>{ SDL_GetPrefPath("opeu", "Operation Europe") } };
  if (!pref) return std::nullopt;
  return std::filesystem::path{ std::string_view{ pref.get() } };
}

// The directory of the executable; nothing when SDL cannot tell.
auto BasePath() -> std::optional<std::filesystem::path> {
  auto const* const base{ SDL_GetBasePath() }; // SDL's own string, alive until SDL_Quit
  if (base == nullptr) return std::nullopt;
  return std::filesystem::path{ std::string_view{ base } };
}
}

auto DefaultSaveDir() -> std::string {
  return PreferencePath().value_or(std::filesystem::path{ "." }).string();
}

auto DataDirectories() -> std::vector<std::filesystem::path> {
  auto places{ std::vector<std::filesystem::path>{} };
  if (auto const base{ BasePath() }) places.push_back(*base);
  if (auto const pref{ PreferencePath() }) places.push_back(*pref / "data");
  return places;
}

auto PrepareSaveDir(std::string const& path) -> void {
  SDL_CreateDirectory(path.c_str()); // a first run's --saves DIR
}

auto ShowMissingData(std::string_view const report) -> void {
  auto const message{ std::string{ report } };
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Operation Europe", message.c_str(), nullptr); // no window of its own yet
}

}
