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

#ifndef COMPONENTS_LOCAL_STORAGE_MANAGER_BROWSER_LOCAL_STORAGE_MANAGER_FAKE_IMPL_H_
#define COMPONENTS_LOCAL_STORAGE_MANAGER_BROWSER_LOCAL_STORAGE_MANAGER_FAKE_IMPL_H_

#include "base/memory/singleton.h"
#include "components/local_storage_manager/public/local_storage_manager.h"

namespace content {

class LocalStorageManagerFakeImpl final : public LocalStorageManager {
 public:
  static LocalStorageManagerFakeImpl* GetInstance();

  void Initialize(const base::FilePath& data_file_path) override{};

  void OnAppInstalled(const std::string& app_id) override{};
  void OnAppRemoved(const std::string& app_id) override{};
  void OnAccessOrigin(const std::string& app_id,
                      const GURL& origin,
                      base::OnceCallback<void()> callback) override;

  base::WeakPtr<LocalStorageManager> GetWeakPtr() override;

 private:
  friend struct base::DefaultSingletonTraits<LocalStorageManagerFakeImpl>;
  LocalStorageManagerFakeImpl(){};

  DISALLOW_COPY_AND_ASSIGN(LocalStorageManagerFakeImpl);
};

}  // namespace content

#endif  // COMPONENTS_LOCAL_STORAGE_MANAGER_BROWSER_LOCAL_STORAGE_MANAGER_FAKE_IMPL_H_
