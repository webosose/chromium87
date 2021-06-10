// Copyright 2021 LG Electronics, Inc.
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

#include "extensions/shell/neva/webos_shutdown_signal_handler.h"

#include <limits.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <utility>

#include "base/callback.h"
#include "base/debug/leak_annotations.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/posix/eintr_wrapper.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_task_runner_handle.h"

namespace {

pid_t g_pipe_pid = -1;
int g_shutdown_pipe_write_fd = -1;
int g_shutdown_pipe_read_fd = -1;

// Common code between SIG{HUP, INT, TERM}Handler.
void GracefulShutdownHandler(int signal) {
  LOG(INFO) << __func__ << " shutdown for signal: " << signal;

  // Reinstall the default handler.  We had one shot at graceful shutdown.
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIG_DFL;
  RAW_CHECK(sigaction(signal, &action, NULL) == 0);

  RAW_CHECK(g_pipe_pid != -1);
  RAW_CHECK(g_shutdown_pipe_write_fd != -1);
  RAW_CHECK(g_shutdown_pipe_read_fd != -1);
  RAW_CHECK(g_pipe_pid == getpid());
  size_t bytes_written = 0;
  do {
    int rv = HANDLE_EINTR(
        write(g_shutdown_pipe_write_fd,
              reinterpret_cast<const char*>(&signal) + bytes_written,
              sizeof(signal) - bytes_written));
    RAW_CHECK(rv >= 0);
    bytes_written += rv;
  } while (bytes_written < sizeof(signal));
}

void SIGHUPHandler(int signal) {
  RAW_CHECK(signal == SIGHUP);
  GracefulShutdownHandler(signal);
}

void SIGINTHandler(int signal) {
  RAW_CHECK(signal == SIGINT);
  GracefulShutdownHandler(signal);
}

void SIGTERMHandler(int signal) {
  RAW_CHECK(signal == SIGTERM);
  GracefulShutdownHandler(signal);
}

// Runs a thread that invokes a callback when a termination signal handler
// is invoked. Uses a pipe to wait for the signal handler to run.
class AppShellShutdownHandler : public base::PlatformThread::Delegate {
 public:
  AppShellShutdownHandler(
      int shutdown_fd,
      base::OnceCallback<void(int)> shutdown_callback,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
  ~AppShellShutdownHandler() override;

  // base::PlatformThread::Delegate:
  void ThreadMain() override;

 private:
  const int shutdown_fd_;
  base::OnceCallback<void(int)> shutdown_callback_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(AppShellShutdownHandler);
};

AppShellShutdownHandler::AppShellShutdownHandler(
    int shutdown_fd,
    base::OnceCallback<void(int)> shutdown_callback,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : shutdown_fd_(shutdown_fd),
      shutdown_callback_(std::move(shutdown_callback)),
      task_runner_(task_runner) {
  CHECK_NE(shutdown_fd_, -1);
  CHECK(!shutdown_callback_.is_null());
  CHECK(task_runner_);
}

AppShellShutdownHandler::~AppShellShutdownHandler() {}

// These functions are used to help us diagnose crash dumps that happen
// during the shutdown process.
NOINLINE void ShutdownFDReadError() {
  // Ensure function isn't optimized away.
  asm("");
  sleep(UINT_MAX);
}

NOINLINE void ShutdownFDClosedError() {
  // Ensure function isn't optimized away.
  asm("");
  sleep(UINT_MAX);
}

NOINLINE void ExitPosted() {
  // Ensure function isn't optimized away.
  asm("");
  sleep(UINT_MAX);
}

void AppShellShutdownHandler::ThreadMain() {
  VLOG(1) << __func__ << " Start.";

  base::PlatformThread::SetName("AppShellShutdownHandler");

  int signal;
  size_t bytes_read = 0;
  ssize_t ret;
  do {
    ret = HANDLE_EINTR(read(shutdown_fd_,
                            reinterpret_cast<char*>(&signal) + bytes_read,
                            sizeof(signal) - bytes_read));
    if (ret < 0) {
      NOTREACHED() << "Unexpected error: " << strerror(errno);
      ShutdownFDReadError();
      break;
    } else if (ret == 0) {
      NOTREACHED() << "Unexpected closure of shutdown pipe.";
      ShutdownFDClosedError();
      break;
    }
    bytes_read += ret;
  } while (bytes_read < sizeof(signal));

  if (!task_runner_->PostTask(
          FROM_HERE, base::BindOnce(std::move(shutdown_callback_), signal))) {
    // Without a valid task runner to post the exit task to, there aren't many
    // options. Raise the signal again. The default handler will pick it up
    // and cause an ungraceful exit.
    RAW_LOG(WARNING, "No valid task runner, exiting ungracefully.");
    kill(getpid(), signal);

    // The signal may be handled on another thread.  Give that a chance to
    // happen.
    sleep(3);

    // We really should be dead by now.  For whatever reason, we're not. Exit
    // immediately, with the exit status set to the signal number with bit 8
    // set.  On the systems that we care about, this exit status is what is
    // normally used to indicate an exit by this signal's default handler.
    // This mechanism isn't a de jure standard, but even in the worst case, it
    // should at least result in an immediate exit.
    RAW_LOG(WARNING, "Still here, exiting really ungracefully.");
    _exit(signal | (1 << 7));
  }
  ExitPosted();
}

}  // namespace

void InstallShutdownSignalHandlers(
    base::OnceCallback<void(int)> shutdown_callback,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  LOG(INFO) << __func__;

  int pipefd[2];
  int ret = pipe(pipefd);
  if (ret < 0) {
    PLOG(DFATAL) << "Failed to create pipe";
    return;
  }

  g_pipe_pid = getpid();
  g_shutdown_pipe_read_fd = pipefd[0];
  g_shutdown_pipe_write_fd = pipefd[1];

  const size_t kShutdownHandlerThreadStackSize = PTHREAD_STACK_MIN * 2;
  AppShellShutdownHandler* detector = new AppShellShutdownHandler(
      g_shutdown_pipe_read_fd, std::move(shutdown_callback), task_runner);
  // PlatformThread does not delete its delegate.
  ANNOTATE_LEAKING_OBJECT_PTR(detector);
  if (!base::PlatformThread::CreateNonJoinable(kShutdownHandlerThreadStackSize,
                                               detector)) {
    LOG(DFATAL) << "Failed to create shutdown detector task.";
  }

  // We need to handle SIGTERM/SIGKILL, because that is how webOS ask
  // Enact Browser processes to quit gracefully at shutdown time.
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIGTERMHandler;
  CHECK_EQ(0, sigaction(SIGTERM, &action, nullptr));

  // Also handle SIGINT - when the user terminates the browser via Ctrl+C. If
  // the browser process is being debugged, GDB will catch the SIGINT first.
  action.sa_handler = SIGINTHandler;
  CHECK_EQ(0, sigaction(SIGINT, &action, nullptr));

  // And SIGHUP, for when the terminal disappears. On shutdown, many Linux
  // distros send SIGHUP, SIGTERM, and then SIGKILL.
  action.sa_handler = SIGHUPHandler;
  CHECK_EQ(0, sigaction(SIGHUP, &action, nullptr));
}
