// Copyright 2019 LG Electronics, Inc.
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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_QUOTA_PERMISSION_CONTEXT_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_QUOTA_PERMISSION_CONTEXT_H_

#include "content/public/browser/quota_permission_context.h"

namespace neva_app_runtime {

class AppRuntimeQuotaPermissionDelegate;

class AppRuntimeQuotaPermissionContext
    : public content::QuotaPermissionContext {
 public:
  AppRuntimeQuotaPermissionContext(AppRuntimeQuotaPermissionDelegate* delegate);
  void RequestQuotaPermission(const content::StorageQuotaParams& params,
                              int render_process_id,
                              PermissionCallback callback) override;

 private:
  ~AppRuntimeQuotaPermissionContext() override {}

  AppRuntimeQuotaPermissionDelegate* delegate_;
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_QUOTA_PERMISSION_CONTEXT_H_
