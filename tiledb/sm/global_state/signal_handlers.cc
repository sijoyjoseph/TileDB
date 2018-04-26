/**
 * @file   signal_handlers.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2018 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 This file defines signal handling functionality.
 */

#include <atomic>
#include <mutex>
#include <string>

#ifdef _WIN32
#error "Signal handling unimplemented on Windows"
#else
#include <signal.h>
#include <unistd.h>
#endif

#include "tiledb/sm/global_state/signal_handlers.h"
#include "tiledb/sm/misc/logger.h"

namespace tiledb {
namespace sm {
namespace global_state {

/* ********************************* */
/*          Global variables         */
/* ********************************* */

/**
 * Singleton SignalHandlers instance.
 *
 * Note: because TileDB's test executable is linked both against the TileDB
 * dynamic object and the core object files themselves, we can have two copies
 * of static globals. This is not necessarily a problem, just something to
 * remember.
 */
SignalHandlers globalSignalHandlers;

std::atomic_bool signal_received(false);

/* ********************************* */
/*     Platform-neutral functions    */
/* ********************************* */

bool SignalHandlers::signal_received() {
  bool test = true;
  return tiledb::sm::global_state::signal_received.compare_exchange_weak(
      test, false);
}

#ifdef _WIN32
/* ********************************* */
/*       Win32 implementations       */
/* ********************************* */
#error "Signal handling unimplemented on Windows"
#else
/* ********************************* */
/*       POSIX implementations       */
/* ********************************* */

/**
 * Signal handler function.
 * @param signum Signal number being handled.
 */
static void signal_handler(int signum) {
  switch (signum) {
    case SIGINT:
      signal_received = true;
      break;
  }
}

SignalHandlers::SignalHandlers() {
  struct sigaction action;
  sigemptyset(&action.sa_mask);
  // Block additional SIGINTs while in the SIGINT handler:
  sigaddset(&action.sa_mask, SIGINT);
  action.sa_flags = 0;
  action.sa_handler = signal_handler;
  if (sigaction(SIGINT, &action, NULL) != 0) {
    LOG_ERROR(
        std::string("Failed to install SIGINT handler: ") + strerror(errno));
  }
}

void SignalHandlers::safe_stderr(const char* msg, size_t msg_len) {
  write(2, msg, msg_len);
}

#endif

}  // namespace global_state
}  // namespace sm
}  // namespace tiledb