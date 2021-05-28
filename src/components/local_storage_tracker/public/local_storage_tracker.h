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

#ifndef COMPONENTS_LOCAL_STORAGE_TRACKER_PUBLIC_LOCAL_STORAGE_TRACKER_H_
#define COMPONENTS_LOCAL_STORAGE_TRACKER_PUBLIC_LOCAL_STORAGE_TRACKER_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/local_storage_tracker/common/local_storage_tracker_store.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace net {
class URLRequest;
}

namespace content {

class LocalStorageTracker
    : public base::RefCountedThreadSafe<LocalStorageTracker> {
 public:
  LocalStorageTracker() = default;
  LocalStorageTracker(const LocalStorageTracker&) = delete;
  LocalStorageTracker& operator=(const LocalStorageTracker&) = delete;
  virtual ~LocalStorageTracker() = default;

  static std::unique_ptr<LocalStorageTracker> Create();
  virtual void Initialize(const base::FilePath& data_file_path) = 0;
  virtual void OnAccessOrigin(const std::string& app_id,
                              const GURL& origin,
                              base::OnceCallback<void()> callback) = 0;

  virtual base::WeakPtr<LocalStorageTracker> GetWeakPtr() = 0;

  virtual void OnAppInstalled(const std::string& app_id) = 0;
  virtual void OnAppRemoved(const std::string& app_id) = 0;

  friend class base::RefCountedThreadSafe<LocalStorageTracker>;
};

}  // namespace content

#endif  // COMPONENTS_LOCAL_STORAGE_TRACKER_PUBLIC_LOCAL_STORAGE_TRACKER_H_
