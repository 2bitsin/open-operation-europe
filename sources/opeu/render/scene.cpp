#include "opeu/render/scene.hpp"

#include <utility>

namespace opeu::render::detail::scene {

auto Scene::Add(Command command) -> Scene& {
  _commands.push_back(std::move(command));
  return *this;
}

auto Scene::Tint(Rgba const tint) -> Scene& {
  _tint = tint;
  return *this;
}

}
