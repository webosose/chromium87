// Copyright (c) 2020 LG Electronics, Inc.
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

#include "components/local_storage_manager/public/local_storage_manager.h"

#include <memory>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "components/local_storage_manager/browser/local_storage_manager_fake_impl.h"
#include "components/local_storage_manager/browser/local_storage_manager_impl.h"

namespace content {

namespace local_storage_manager {

bool IsFeatureEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kLocalStorageManager);
}
}  // namespace local_storage_manager

LocalStorageManagerImpl* LocalStorageManagerImpl::GetInstance() {
  return base::Singleton<LocalStorageManagerImpl>::get();
}

LocalStorageManagerFakeImpl* LocalStorageManagerFakeImpl::GetInstance() {
  return base::Singleton<LocalStorageManagerFakeImpl>::get();
}

std::unique_ptr<LocalStorageManager> LocalStorageManager::Create() {
  if (local_storage_manager::IsFeatureEnabled()) {
    VLOG(1) << "Local Storage Monitor feature is enabled - using "
               "LocalStorageManagerImpl";
    return std::unique_ptr<LocalStorageManagerImpl>(
        LocalStorageManagerImpl::GetInstance());
  } else {
    VLOG(1) << "Local Storage Monitor feature is disabled - using "
               "LocalStorageManagerFakeImpl";
    return std::unique_ptr<LocalStorageManagerFakeImpl>(
        LocalStorageManagerFakeImpl::GetInstance());
  }
}

}  // namespace content
