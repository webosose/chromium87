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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_SHARED_MEMORY_MANAGER_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_SHARED_MEMORY_MANAGER_H_

#include "base/compiler_specific.h"
#include "base/memory/memory_pressure_listener.h"

namespace discardable_memory {
class DiscardableSharedMemoryManager;
}  // namespace discardable_memory

namespace neva_app_runtime {

class AppRuntimeSharedMemoryManager {
 public:
  AppRuntimeSharedMemoryManager();
  AppRuntimeSharedMemoryManager(const AppRuntimeSharedMemoryManager&) = delete;
  AppRuntimeSharedMemoryManager& operator=(
      const AppRuntimeSharedMemoryManager&) = delete;
  ~AppRuntimeSharedMemoryManager();

 private:
  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  size_t memory_pressure_divider_ = 4;
  size_t minimal_limit_ = 8 * 1024 * 1024;
  size_t memory_limit_;
  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;
  discardable_memory::DiscardableSharedMemoryManager*
      discardable_shared_memory_manager_ = nullptr;
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_SHARED_MEMORY_MANAGER_H_
