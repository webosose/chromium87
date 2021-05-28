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

#ifndef COMPONENTS_LOCAL_STORAGE_TRACKER_BROWSER_LOCAL_STORAGE_TRACKER_FAKE_IMPL_H_
#define COMPONENTS_LOCAL_STORAGE_TRACKER_BROWSER_LOCAL_STORAGE_TRACKER_FAKE_IMPL_H_

#include "base/memory/singleton.h"
#include "components/local_storage_tracker/public/local_storage_tracker.h"

namespace content {

class LocalStorageTrackerFakeImpl final : public LocalStorageTracker {
 public:
  LocalStorageTrackerFakeImpl(const LocalStorageTrackerFakeImpl&) = delete;
  LocalStorageTrackerFakeImpl& operator=(const LocalStorageTrackerFakeImpl&) =
      delete;

  static LocalStorageTrackerFakeImpl* GetInstance();

  void Initialize(const base::FilePath& data_file_path) override{};

  void OnAppInstalled(const std::string& app_id) override{};
  void OnAppRemoved(const std::string& app_id) override{};
  void OnAccessOrigin(const std::string& app_id,
                      const GURL& origin,
                      base::OnceCallback<void()> callback) override;

  base::WeakPtr<LocalStorageTracker> GetWeakPtr() override;

 private:
  LocalStorageTrackerFakeImpl() = default;

  friend struct base::DefaultSingletonTraits<LocalStorageTrackerFakeImpl>;
};

}  // namespace content

#endif  // COMPONENTS_LOCAL_STORAGE_TRACKER_BROWSER_LOCAL_STORAGE_TRACKER_FAKE_IMPL_H_
