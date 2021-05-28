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

#ifndef COMPONENTS_LOCAL_STORAGE_TRACKER_BROWSER_LOCAL_STORAGE_TRACKER_URL_REQUEST_HANDLER_H_
#define COMPONENTS_LOCAL_STORAGE_TRACKER_BROWSER_LOCAL_STORAGE_TRACKER_URL_REQUEST_HANDLER_H_

#include "base/memory/ref_counted.h"
#include "components/local_storage_tracker/public/local_storage_tracker.h"
#include "net/base/completion_once_callback.h"

namespace net {
class URLRequest;
}

namespace content {

class LocalStorageTracker;

class LocalStorageTrackerUrlRequestHandler
    : public base::RefCountedThreadSafe<LocalStorageTrackerUrlRequestHandler> {
 public:
  LocalStorageTrackerUrlRequestHandler(
      base::WeakPtr<LocalStorageTracker> local_storage_tracker);
  void OnAccessOrigin(content::WebContents* web_contents,
                      const GURL& origin,
                      base::OnceCallback<void()> callback) const;
  int OnBeforeURLRequest(net::URLRequest* request,
                         net::CompletionOnceCallback& callback,
                         GURL* new_url);

 private:
  friend class base::RefCountedThreadSafe<LocalStorageTrackerUrlRequestHandler>;
  bool DoesRequestAffectStorage(net::URLRequest* request) const;
  base::WeakPtr<LocalStorageTracker> local_storage_tracker_;
  bool local_storage_tracker_valid_;
};

}  // namespace content

#endif  // COMPONENTS_LOCAL_STORAGE_TRACKER_BROWSER_LOCAL_STORAGE_TRACKER_URL_REQUEST_HANDLER_H_
