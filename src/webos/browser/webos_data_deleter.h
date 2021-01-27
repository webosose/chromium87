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

#ifndef WEBOS_BROWSER_WEBOS_DATA_DELETER_H_
#define WEBOS_BROWSER_WEBOS_DATA_DELETER_H_

#include <set>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/data_deleter.h"
#include "neva/app_runtime/browser/browsing_data/browsing_data_remover.h"
#include "neva/app_runtime/webview_profile.h"
#include "url/origin.h"

namespace webos {

class WebOSDataDeleter final : public content::DataDeleter {
 public:
  WebOSDataDeleter();

  typedef std::set<GURL> Origins;

  void StartDeleting(
      const GURL& origin,
      bool delete_cookies,
      content::DataDeleter::CompletionCallback callback) override;

  void OnDeleteCompleted(
      const GURL& origin,
      scoped_refptr<content::DataDeleter::DeletionContext> context) override;

 private:
  const neva_app_runtime::BrowsingDataRemover::RemoveBrowsingDataMask mask_;

  base::WeakPtrFactory<WebOSDataDeleter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebOSDataDeleter);
};
}  // namespace webos

#endif  // WEBOS_BROWSER_WEBOS_DATA_DELETER_H_
