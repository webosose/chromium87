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

#include "webos/browser/webos_data_deleter.h"

#include "base/lazy_instance.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "neva/app_runtime/webview_profile.h"

namespace webos {

WebOSDataDeleter::WebOSDataDeleter()
    : weak_factory_(this),
      mask_(static_cast<
            neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask>(
          neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask::
              REMOVE_LOCAL_STORAGE |
          neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask::
              REMOVE_INDEXEDDB |
          neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask::
              REMOVE_WEBSQL |
          neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask::
              REMOVE_APPCACHE |
          neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask::
              REMOVE_SERVICE_WORKERS |
          neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask::
              REMOVE_CACHE_STORAGE |
          neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask::
              REMOVE_FILE_SYSTEMS |
          neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask::
              REMOVE_MEDIA_LICENSES)) {}

void WebOSDataDeleter::StartDeleting(
    const GURL& origin,
    bool delete_cookies,
    content::DataDeleter::CompletionCallback callback) {
  auto profile = static_cast<neva_app_runtime::WebViewProfile*>(
      neva_app_runtime::WebViewProfile::GetDefaultProfile());
  int mask_value = static_cast<int>(mask_);
  if (delete_cookies) {
    mask_value |= neva_app_runtime::BrowsingDataRemover::
        RemoveBrowsingDataMask::REMOVE_COOKIES;
  }
  neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask mask =
      static_cast<
          neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask>(
          mask_value);
  profile->RemoveBrowsingData(mask, origin, std::move(callback));
}

void WebOSDataDeleter::OnDeleteCompleted(
    const GURL& origin,
    scoped_refptr<content::DataDeleter::DeletionContext> context) {
  if (!context)
    return;
  std::set<GURL>::iterator it = context->origins_.find(origin);
  if (it != context->origins_.end()) {
    context->origins_.erase(it);
  }
  if (context->origins_.size() == 0) {
    base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                   std::move(context->callback_));
  }
}

}  // namespace webos
