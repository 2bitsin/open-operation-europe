#include "core/presenters.hpp"

#include "common/dispatch.hpp"
#include "common/game.hpp"
#include "core/aircargo.hpp"
#include "core/airstrk.hpp"
#include "core/arrshow.hpp"
#include "core/daypres.hpp"
#include "core/evangel.hpp"
#include "core/evbomb.hpp"
#include "core/evloss.hpp"
#include "core/evpartis.hpp"
#include "core/evreport.hpp"
#include "core/evspy.hpp"
#include "core/evvisit.hpp"
#include "core/evwound.hpp"
#include "core/msnrept.hpp"
#include "core/notend.hpp"
#include "core/notice.hpp"
#include "core/paradrop.hpp"
#include "core/persmenu.hpp"
#include "core/reqerror.hpp"
#include "core/reqpend.hpp"
#include "core/rqcommon.hpp"
#include "core/specops.hpp"
#include "map/blast.hpp"
#include "map/movespot.hpp"
#include "units/allocdst.hpp"
#include "units/cmdxfer.hpp"
#include "units/engineer.hpp"
#include "units/ordattk.hpp"
#include "units/ordbridg.hpp"
#include "units/xferstep.hpp"

namespace opeu::game::detail::presenters {
namespace {
// The presenters' routines, by their enumerator.
constexpr auto PRESENTERS = DispatchRows<Presenter, auto (*)(Game&)->void>({
    { Presenter::DAILY_EVENT,       opeu_daily_present            },
    { Presenter::MISSION_FINISH,    opeu_mission_finish_present   },
    { Presenter::MISSION_DELAY,     opeu_mission_delay_present    },
    { Presenter::AIR_STRIKE,        opeu_air_strike_present       },
    { Presenter::PARATROOPERS,      opeu_para_drop_present        },
    { Presenter::CARGO_DROP,        opeu_cargo_drop_present       },
    { Presenter::VISIT,             opeu_visit_present            },
    { Presenter::HURRICANE,         opeu_hurricane_present        },
    { Presenter::PARTISAN_LOSSES,   opeu_loss_present_a           },
    { Presenter::NORTHERN_LOSSES,   opeu_loss_present_b           },
    { Presenter::BOMBING,           opeu_bomb_present             },
    { Presenter::ANGEL,             opeu_angel_present            },
    { Presenter::SPY_GREETING,      opeu_spy_present_a            },
    { Presenter::SPY_ENDING,        opeu_spy_present_b            },
    { Presenter::REPORT,            opeu_report_present           },
    { Presenter::DEATH,             opeu_death_present            },
    { Presenter::REQUEST_ERROR,     opeu_request_error_present    },
    { Presenter::REQUEST_START,     opeu_pending_start_present    },
    { Presenter::REQUEST_FALLBACK,  opeu_pending_fallback_present },
    { Presenter::ARRIVAL,           opeu_arrival_present          },
    { Presenter::BLAST_PAUSE,       opeu_blast_pause              },
    { Presenter::TRANSFER,          opeu_transfer_present         },
    { Presenter::PERSONNEL_FINISH,  opeu_personnel_finish         },
    { Presenter::ALLOCATION_FINISH, opeu_alloc_finish             },
    { Presenter::REQUEST_FINISH,    opeu_request_finish           },
    { Presenter::SPECIAL_FORCES,    opeu_special_present          },
    { Presenter::ATTACK,            opeu_order_attack_present     },
    { Presenter::OCCUPIED,          opeu_order_occupied_present   },
    { Presenter::BRIDGE_DESTROYED,  opeu_order_destroy_present    },
    { Presenter::MINES_LAID,        opeu_mines_laid_finish        },
    { Presenter::MINES_REMOVED,     opeu_mines_removed_finish     },
    { Presenter::BRIDGE_BUILT,      opeu_bridge_built_finish      },
    { Presenter::ARMS_REPAIRED,     opeu_arms_repaired_finish     },
    { Presenter::TRANSFER_STEP,     opeu_transfer_step            },
    { Presenter::MINES_HIT,         opeu_move_mines_finish        },
    { Presenter::NOTICE_VIEW,       opeu_notice_view_finish       },
    { Presenter::END_OCCUPY,        opeu_end_occupy_present       },
});
} // namespace

auto RunPresenter(Game& game, Presenter presenter) -> void {
  Dispatch(PRESENTERS, presenter, game);
}
} // namespace opeu::game::detail::presenters
