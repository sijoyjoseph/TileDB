#include <signal.h>
#include <tiledb/sm/misc/constants.h>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include <mutex>
#include <string>

#include "global_state.h"
#include "tiledb/sm/global_state/signal_handlers.h"
#include "tiledb/sm/global_state/watchdog.h"
#include "tiledb/sm/misc/constants.h"
#include "tiledb/sm/misc/logger.h"

namespace tiledb {
namespace sm {
namespace global_state {

/** Singleton Watchdog instance. */
Watchdog globalWatchdog;

Watchdog::Watchdog() {
  should_exit_ = false;
  thread_ = std::thread([this]() { watchdog_thread(this); });
}

Watchdog::~Watchdog() {
  {
    std::unique_lock<std::mutex> lck(mtx_);
    should_exit_ = true;
    cv_.notify_one();
  }
  thread_.join();
}

void Watchdog::watchdog_thread(Watchdog* watchdog) {
  if (watchdog == nullptr) {
    return;
  }

  while (true) {
    std::unique_lock<std::mutex> lck(watchdog->mtx_);
    watchdog->cv_.wait_for(
        lck, std::chrono::milliseconds(constants::watchdog_thread_sleep_ms));
    std::cerr << "Watchdog awake" << std::endl;

    if (SignalHandlers::signal_received()) {
      std::cerr << "Watchdog: got signal" << std::endl;
      for (auto* sm : GlobalState::GetGlobalState().storage_managers()) {
        sm->cancel_all_tasks();
      }
    }

    if (watchdog->should_exit_) {
      std::cerr << "Normal watchdog termination" << std::endl;
      break;
    }
  }
}

}  // namespace global_state
}  // namespace sm
}  // namespace tiledb