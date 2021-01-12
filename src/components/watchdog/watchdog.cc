// Copyright 2016-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "components/watchdog/watchdog.h"

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "base/logging.h"

namespace watchdog {

static constexpr int kDefaultWatchdogTimeout = 100;
static constexpr int kDefaultWatchdogPeriod = 20;
static constexpr int kWatchdogCleanupPeriod = 10;

__attribute__((noinline)) static void _callstack_signal_handler(int signal,
                                                                siginfo_t* info,
                                                                void* secret) {
  if (signal != SIGUSR2)
    return;

  LOG(ERROR) << "[Watchdog_SigHandler] Stuck handling at thread : "
             << syscall(SYS_gettid);

  constexpr int kMaxFrames = 128;
  void* callstack[kMaxFrames];
  constexpr size_t kBufferSize = 1024;
  char buffer[kBufferSize];
  int nFrames = backtrace(callstack, kMaxFrames);
  char** symbols = backtrace_symbols(callstack, nFrames);

  for (int i = 1; i < nFrames; i++) {
    snprintf(buffer, kBufferSize, "%s", symbols[i]);
    LOG(ERROR) << "[Watchdog_Callstack] " << buffer;
  }

  free(symbols);
  if (nFrames == kMaxFrames)
    LOG(ERROR) << "[Watchdog_Callstack] [truncated]";

  kill(getpid(), SIGABRT);
}

Watchdog::Watchdog()
    : period_(kDefaultWatchdogPeriod),
      timeout_(kDefaultWatchdogTimeout),
      watching_pthread_id_(0),
      watching_thread_tid_(0) {}

Watchdog::~Watchdog() {
  if (watchdog_thread_) {
    watchdog_thread_->Disarm();
    watchdog_thread_->Cleanup();
  }
}

void Watchdog::StartWatchdog() {
  watchdog_thread_.reset(
      new WatchdogThread(base::TimeDelta::FromSeconds(timeout_), this));
}

void Watchdog::Arm() {
  watchdog_thread_->Arm();
}

Watchdog::WatchdogThread::WatchdogThread(const base::TimeDelta& duration,
                                         watchdog::Watchdog* watchdog)
    : base::Watchdog(duration, "Watchdog", true), watchdog_(watchdog) {}

void Watchdog::WatchdogThread::Alarm() {
  LOG(ERROR) << "[WatchdogThread] Detected stuck thread "
             << watchdog_->GetWatchingThreadTid() << " in process " << getpid()
             << "! Killing process with SIGABRT";

  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = _callstack_signal_handler;
  sigaction(SIGUSR2, &sa, NULL);

  if (pthread_kill(watchdog_->GetWatchingPthreadId(), SIGUSR2) != 0) {
    LOG(ERROR) << "[WatchdogThread] "
               << "Cannot send signal!! Process cannot be recovered!!";
  }
}

void Watchdog::SetCurrentThreadInfo() {
  watching_pthread_id_ = pthread_self();
  watching_thread_tid_ = syscall(SYS_gettid);
}

bool Watchdog::HasThreadInfo() const {
  return watching_pthread_id_ && watching_thread_tid_;
}

}  // namespace watchdog
