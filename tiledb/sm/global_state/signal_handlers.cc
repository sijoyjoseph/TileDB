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

#include <signal.h>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
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
 * Flag set to true from the installed signal handlers. This is a global
 * variable in the interest of being paranoid about what can be modified safely
 * from a signal handler.
 */
std::atomic_bool signal_received(false);

/* ********************************* */
/*     Platform-neutral functions    */
/* ********************************* */

SignalHandlers& SignalHandlers::GetSignalHandlers() {
  // This is thread-safe in C++11.
  static SignalHandlers signalHandlers;
  return signalHandlers;
}

bool SignalHandlers::signal_received() {
  bool test = true;
  return tiledb::sm::global_state::signal_received.compare_exchange_weak(
      test, false);
}

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

#ifdef _WIN32
/* ********************************* */
/*       Win32 implementations       */
/* ********************************* */

static BOOL win_ctrl_handler(DWORD dwCtrlType) {
  switch (dwCtrlType) {
    case CTRL_BREAK_EVENT:
      signal_handler(SIGINT);
      break;
  }
  return false;
}

Status SignalHandlers::initialize() {
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    return Status::Error(
        std::string("Failed to install Win32 SIGINT handler: ") +
        strerror(errno));
  }

  // Win32 applications should also handle Ctrl-Break.
  if (SetConsoleCtrlHandler(win_ctrl_handler, true) == 0) {
    return Status::Error(std::string("Failed to install Win32 ctrl handler"));
  }
  return Status::Ok();
}

void SignalHandlers::safe_stderr(const char* msg, size_t msg_len) {
  auto retval = _write(2, msg, (unsigned int)msg_len);
  // Ignore return value.
  (void)retval;
}

#else
/* ********************************* */
/*       POSIX implementations       */
/* ********************************* */

Status SignalHandlers::initialize() {
  struct sigaction action;
  sigemptyset(&action.sa_mask);
  // Block additional SIGINTs while in the SIGINT handler:
  sigaddset(&action.sa_mask, SIGINT);
  action.sa_flags = 0;
  action.sa_handler = signal_handler;
  if (sigaction(SIGINT, &action, NULL) != 0) {
    return Status::Error(
        std::string("Failed to install SIGINT handler: ") + strerror(errno));
  }
  return Status::Ok();
}

void SignalHandlers::safe_stderr(const char* msg, size_t msg_len) {
  auto retval = write(2, msg, msg_len);
  // Ignore return value.
  (void)retval;
}

#endif

}  // namespace global_state
}  // namespace sm
}  // namespace tiledb
