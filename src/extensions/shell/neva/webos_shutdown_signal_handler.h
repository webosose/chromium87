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

#ifndef EXTENSIONS_SHELL_NEVA_WEBOS_SHUTDOWN_SIGNAL_HANDLER_H_
#define EXTENSIONS_SHELL_NEVA_WEBOS_SHUTDOWN_SIGNAL_HANDLER_H_

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"

namespace base {
class SingleThreadTaskRunner;
}

// Runs a background thread that installs signal handlers to watch for shutdown
// signals like SIGTERM, SIGINT and SIGTERM. |shutdown_callback| is invoked on
// |task_runner| which is usually the main thread's task runner.
void InstallShutdownSignalHandlers(
    base::OnceCallback<void(int)> shutdown_callback,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);

#endif  // EXTENSIONS_SHELL_NEVA_WEBOS_SHUTDOWN_SIGNAL_HANDLER_H_
