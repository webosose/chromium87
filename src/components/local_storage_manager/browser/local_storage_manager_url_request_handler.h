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

#ifndef COMPONENTS_LOCAL_STORAGE_MANAGER_BROWSER_LOCAL_STORAGE_MANAGER_URL_REQUEST_HANDLER_H_
#define COMPONENTS_LOCAL_STORAGE_MANAGER_BROWSER_LOCAL_STORAGE_MANAGER_URL_REQUEST_HANDLER_H_

#include "base/memory/ref_counted.h"
#include "components/local_storage_manager/public/local_storage_manager.h"
#include "net/base/completion_once_callback.h"

namespace net {
class URLRequest;
}

namespace content {

class LocalStorageManager;

class LocalStorageManagerUrlRequestHandler
    : public base::RefCountedThreadSafe<LocalStorageManagerUrlRequestHandler> {
 public:
  LocalStorageManagerUrlRequestHandler(
      base::WeakPtr<LocalStorageManager> local_storage_manager);
  void OnAccessOrigin(content::WebContents* web_contents,
                      const GURL& origin,
                      base::OnceCallback<void()> callback) const;
  int OnBeforeURLRequest(net::URLRequest* request,
                         net::CompletionOnceCallback& callback,
                         GURL* new_url);

 private:
  friend class base::RefCountedThreadSafe<LocalStorageManagerUrlRequestHandler>;
  bool DoesRequestAffectStorage(net::URLRequest* request) const;
  base::WeakPtr<LocalStorageManager> local_storage_manager_;
  bool local_storage_manager_valid_;
};
}  // namespace content

#endif  // COMPONENTS_LOCAL_STORAGE_MANAGER_BROWSER_LOCAL_STORAGE_MANAGER_URL_REQUEST_HANDLER_H_
