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

#include "components/local_storage_tracker/browser/local_storage_tracker_fake_impl.h"

#include "content/public/browser/browser_thread.h"

namespace content {

void LocalStorageTrackerFakeImpl::OnAccessOrigin(
    const std::string& app_id,
    const GURL& origin,
    base::OnceCallback<void()> callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  NOTREACHED() << "Should never have been reached here";
  std::move(callback).Run();
}

base::WeakPtr<LocalStorageTracker> LocalStorageTrackerFakeImpl::GetWeakPtr() {
  return base::WeakPtr<LocalStorageTracker>();
};

}  // namespace content
