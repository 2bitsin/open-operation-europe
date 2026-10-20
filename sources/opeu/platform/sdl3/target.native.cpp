// The native targets' side of target.hpp: vsync, SDL_Delay, the preference path for saves.
#include "opeu/platform/sdl3/target.hpp"

#include "opeu/platform/sdl3/handles.hpp"

#include <memory>
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

auto DefaultSaveDir() -> std::string {
  auto const pref{ std::unique_ptr<char, Destroy<SDL_free>>{ SDL_GetPrefPath("opeu", "Operation Europe") } };
  return std::string{ pref ? std::string_view{ pref.get() } : std::string_view{ "." } };
}

auto PrepareSaveDir(std::string const& path) -> void {
  SDL_CreateDirectory(path.c_str()); // a first run's --saves DIR
}

}
