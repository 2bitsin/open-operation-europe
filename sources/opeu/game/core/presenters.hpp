// The presenters a daily show runs after its picture (core/dialogs opeu_daily_show), by their Presenter.
#pragma once

#include "common/events.hpp"
#include "common/presenter.hpp"

namespace opeu::game::detail::presenters {

// Runs the presenter's routine.
auto RunPresenter(Game& game, Presenter presenter) -> void;
} // namespace opeu::game::detail::presenters

namespace opeu::game {
using detail::presenters::RunPresenter;
} // namespace opeu::game
