// Copyright 2019 LG Electronics, Inc.
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

#include "neva/app_runtime/browser/app_runtime_shared_memory_manager.h"

#include "base/command_line.h"
#include "base/process/process_metrics.h"
#include "base/strings/string_number_conversions.h"
#include "components/discardable_memory/service/discardable_shared_memory_manager.h"
#include "content/browser/browser_main_loop.h"
#include "neva/app_runtime/browser/app_runtime_browser_switches.h"

namespace neva_app_runtime {

AppRuntimeSharedMemoryManager::AppRuntimeSharedMemoryManager()
    : memory_pressure_listener_(new base::MemoryPressureListener(
        FROM_HERE, base::Bind(&AppRuntimeSharedMemoryManager::OnMemoryPressure,
                              base::Unretained(this)))),
      discardable_shared_memory_manager_(
          discardable_memory::DiscardableSharedMemoryManager::Get()) {
  // (jani) Should this be based on shmem dir space?
  base::SystemMemoryInfoKB info;
  int64_t system_memory_mb =
      base::GetSystemMemoryInfo(&info) ? info.total / 1024 : 1024;

  // The reduction factor to be applied to overall system memory when
  // calculating the default memory limit to use for discardable memory.
  size_t system_mem_reduction_factor = 10;
  base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
  if (cmd_line.HasSwitch(
          kSharedMemSystemMemReductionFactor)) {
    size_t reduction_factor;
    if (base::StringToSizeT(cmd_line.GetSwitchValueASCII(
                                kSharedMemSystemMemReductionFactor),
                            &reduction_factor))
      system_mem_reduction_factor = reduction_factor;
  }

  system_memory_mb = system_memory_mb / system_mem_reduction_factor;
  memory_limit_ = system_memory_mb * 1024 * 1024;
  discardable_shared_memory_manager_->SetMemoryLimit(memory_limit_);

  if (cmd_line.HasSwitch(kSharedMemPressureDivider)) {
    size_t pressure_divider;
    if (base::StringToSizeT(
            cmd_line.GetSwitchValueASCII(kSharedMemPressureDivider),
            &pressure_divider))
      memory_pressure_divider_ = pressure_divider;
  }
  if (cmd_line.HasSwitch(kSharedMemMinimalLimitMB)) {
    size_t minimal_limit;
    if (base::StringToSizeT(
            cmd_line.GetSwitchValueASCII(kSharedMemMinimalLimitMB),
            &minimal_limit))
      minimal_limit_ = minimal_limit * 1024 * 1024;
  }
}

AppRuntimeSharedMemoryManager::~AppRuntimeSharedMemoryManager() {
}

void AppRuntimeSharedMemoryManager::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  switch (memory_pressure_level) {
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE:
      break;
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE:
      discardable_shared_memory_manager_->SetMemoryLimit(
          std::max(memory_limit_ / memory_pressure_divider_, minimal_limit_));
      break;
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL:
      discardable_shared_memory_manager_->SetMemoryLimit(minimal_limit_);
      break;
  }
}

}  // namespace neva_app_runtime
