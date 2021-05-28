// Copyright 2020 LG Electronics, Inc.
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

#include "components/local_storage_tracker/public/local_storage_tracker.h"

#include <memory>
#include <unistd.h>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "components/local_storage_tracker/browser/local_storage_tracker_fake_impl.h"
#include "components/local_storage_tracker/browser/local_storage_tracker_impl.h"

namespace content {

namespace local_storage_tracker {

bool IsFeatureEnabled() {
  bool disabler_not_found =
      access("/var/lib/wam/Default/local_storage_tracker_disabled", F_OK) != 0;
  return disabler_not_found &&
         base::CommandLine::ForCurrentProcess()->HasSwitch(
             switches::kLocalStorageTracker);
}

}  // namespace local_storage_tracker

LocalStorageTrackerImpl* LocalStorageTrackerImpl::GetInstance() {
  return base::Singleton<LocalStorageTrackerImpl>::get();
}

LocalStorageTrackerFakeImpl* LocalStorageTrackerFakeImpl::GetInstance() {
  return base::Singleton<LocalStorageTrackerFakeImpl>::get();
}

std::unique_ptr<LocalStorageTracker> LocalStorageTracker::Create() {
  if (local_storage_tracker::IsFeatureEnabled()) {
    VLOG(1) << "Local Storage Tracker feature is enabled - using "
               "LocalStorageTrackerImpl";
    return std::unique_ptr<LocalStorageTrackerImpl>(
        LocalStorageTrackerImpl::GetInstance());
  } else {
    VLOG(1) << "Local Storage Tracker feature is disabled - using "
               "LocalStorageTrackerFakeImpl";
    return std::unique_ptr<LocalStorageTrackerFakeImpl>(
        LocalStorageTrackerFakeImpl::GetInstance());
  }
}

}  // namespace content
